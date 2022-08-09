#ifndef QAT_IR_PROPERTIES_HPP
#define QAT_IR_PROPERTIES_HPP

#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"
#include <list>

namespace qat::IR {

class User;

// A use
class Use {
private:
  User       *user;
  u64         uses;
  Vec<String> purposes;

public:
  Use(User *_user, String purpose);
  User       *getUser();
  useit bool  isUser(User *) const;
  useit u64   getUses() const;
  void        addUse(const String &purpose);
  Vec<String> getPurposes();
};

// A usable entity
class Usable {
private:
  std::list<Use *> data;

public:
  Usable();
  u64        getUses(User *user) const;
  void       addUse(User *user, const String &purpose);
  void       removeUser(User *user);
  useit u64  getTotalUses() const;
  useit bool hasUsers() const;
};

// A user entity
class User {
private:
  Vec<Usable *> uses;

public:
  User();
  bool hasUsable(Usable *) const;
  void addUsable(Usable *);
  void dropAllUses();
};

} // namespace qat::IR

#endif