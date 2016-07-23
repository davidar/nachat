#include "ChatWindow.hpp"
#include "ui_ChatWindow.h"

#include <QIcon>
#include <QCloseEvent>

#include "matrix/Room.hpp"

#include "RoomView.hpp"
#include "RoomViewList.hpp"

ChatWindow::ChatWindow(QWidget *parent)
  : QWidget(parent), ui(new Ui::ChatWindow), room_list_(new RoomViewList(this)) {
  ui->setupUi(this);

  setAttribute(Qt::WA_DeleteOnClose);

  connect(ui->room_stack, &QStackedWidget::currentChanged, this, &ChatWindow::current_changed);
  ui->splitter->insertWidget(0, room_list_);
  ui->splitter->setCollapsible(1, false);
  room_list_->hide();

  connect(room_list_, &RoomViewList::activated, [this](matrix::Room &room) {
      auto &view = *rooms_.at(&room);
      ui->room_stack->setCurrentWidget(&view);
      view.setFocus();
    });
  connect(room_list_, &RoomViewList::claimed, this, &ChatWindow::claimed);
  connect(room_list_, &RoomViewList::released, [this](matrix::Room &room) {
      auto it = rooms_.find(&room);
      if(it != rooms_.end()) {
        auto view = it->second;
        ui->room_stack->removeWidget(view);
        // TODO: Pass ownership elsewhere if necessary
        view->setParent(nullptr);
        rooms_.erase(it);
        delete view;
      }
      switch(ui->room_stack->count()) {
      case 0: close(); break;
      case 1: room_list_->hide(); break;
      default: break;
      }
      released(room);
    });
  connect(room_list_, &RoomViewList::pop_out, [this](matrix::Room &room) {
      auto it = rooms_.find(&room);
      auto view = it->second;
      rooms_.erase(it);
      pop_out(room, view);
    });
}

ChatWindow::~ChatWindow() { delete ui; }

void ChatWindow::add(matrix::Room &r, RoomView *v) {
  v->setParent(this);
  rooms_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(&r),
      std::forward_as_tuple(v));
  ui->room_stack->addWidget(v);
  room_list_->add(r);
  if(room_list_->count() == 2) {
    room_list_->show();
  }
  room_list_->activate(r);
}

void ChatWindow::add_or_focus(matrix::Room &room) {
  if(rooms_.find(&room) == rooms_.end()) {
    add(room, new RoomView(room, this));
  } else {
    room_list_->activate(room);
  }
}

void ChatWindow::room_name_changed(matrix::Room &room) {
  room_list_->update_name(room);
  update_title();
}

RoomView *ChatWindow::take(matrix::Room &room) {
  auto it = rooms_.find(&room);
  auto view = it->second;
  rooms_.erase(it);
  released(room);
  return view;
}

void ChatWindow::update_title() {
  if(auto w = ui->room_stack->currentWidget()) {
    setWindowTitle(static_cast<RoomView*>(w)->room().pretty_name());
  } else {
    setWindowTitle("");
  }
}

void ChatWindow::current_changed(int i) {
  (void)i;
  update_title();
}

void ChatWindow::changeEvent(QEvent *e) {   
  QWidget::changeEvent(e);
  if(e->type() == QEvent::ActivationChange && isActiveWindow()) {
    focused();
  }
}

void ChatWindow::closeEvent(QCloseEvent *evt) {
  for(auto &r : rooms_) {
    released(*r.first);
  }
  evt->accept();
}
