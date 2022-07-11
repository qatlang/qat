#ifndef QAT_IR_PROPERTIES_HPP
#define QAT_IR_PROPERTIES_HPP

#include <list>
#include <string>
#include <vector>

namespace qat::IR {

class User;

// A use
class Use {
private:
  User *user;
  unsigned uses;
  std::vector<std::string> purposes;

public:
  Use(User *_user, std::string purpose);
  User *getUser();
  bool isUser(User *) const;
  unsigned getUses() const;
  void addUse(std::string purpose);
  std::vector<std::string> getPurposes();
};

// A usable entity
class Usable {
private:
  std::list<Use *> data;

public:
  Usable();
  unsigned getUses(User *user) const;
  void addUse(User *user, std::string purpose);
  void removeUser(User *user);
  unsigned getTotalUses() const;
  bool hasUsers() const;
};

// A user entity
class User {
private:
  std::vector<Usable *> uses;

public:
  User();
  bool hasUsable(Usable *) const;
  void addUsable(Usable *);
  void dropAllUses();
};

} // namespace qat::IR

#endif