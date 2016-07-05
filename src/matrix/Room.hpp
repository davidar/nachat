#ifndef NATIVE_CHAT_MATRIX_ROOM_H_
#define NATIVE_CHAT_MATRIX_ROOM_H_

#include <vector>
#include <unordered_map>

#include <QString>
#include <QObject>
#include <QUrl>

#include <span.h>

#include "../QStringHash.hpp"

#include "Member.hpp"

namespace matrix {

class Matrix;
class Session;

namespace proto {
struct JoinedRoom;
}

struct Message {
  const Member &sender;
  QString body;
};

struct StateID {
  QString type, key;
};

struct SIDHash {
  size_t operator()(const StateID &sid) const { return qHash(sid.type) ^ qHash(sid.key); }
};

class RoomState {
public:
  RoomState(Room &room) : room_(room) {}

  bool dispatch(const proto::Event &e, bool timeline);
  // Returns true if changes were made

  const QString &name() const { return name_; }
  const QString &canonical_alias() const { return canonical_alias_; }
  gsl::span<const QString> aliases() const { return aliases_; }
  const QString &topic() const { return topic_; }
  const QUrl &avatar() const { return avatar_; }

  std::vector<const Member *> members() const;
  const Member *member(const QString &id) const;

  QString pretty_name() const;
  // Matrix r0.1.0 11.2.2.5 ish (like vector-web)

  QString member_name(const Member &member) const;
  // Matrix r0.1.0 11.2.2.3

private:
  Room &room_;
  QString name_;
  QString canonical_alias_;
  std::vector<QString> aliases_;
  QString topic_;
  QUrl avatar_;
  std::unordered_map<QString, Member, QStringHash> members_by_id_;
  std::unordered_map<QString, std::vector<Member *>, QStringHash> members_by_displayname_;

  void forget_displayname(const Member &member);
};

class Room : public QObject {
  Q_OBJECT

public:
  Room(Matrix &universe, Session &session, QString id);

  Room(const Room &) = delete;
  Room &operator=(const Room &) = delete;

  const Session &session() const { return session_; }
  const QString &id() const { return id_; }
  uint64_t highlight_count() const { return highlight_count_; }
  uint64_t notification_count() const { return notification_count_; }

  const RoomState &state() const { return state_; }

  void load_state(gsl::span<const proto::Event>);
  void dispatch(const proto::JoinedRoom &);

signals:
  void membership_changed(const Member &, Membership);
  void state_changed();
  void message(const Message &);
  void backlog(const RoomState &, gsl::span<const Message>);
  void highlight_count_changed();
  void notification_count_changed();

private:
  Matrix &universe_;
  Session &session_;
  const QString id_;

  RoomState state_;

  uint64_t highlight_count_ = 0, notification_count_ = 0;
};

}

#endif
