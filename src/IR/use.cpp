#include "./use.hpp"

namespace qat::IR {

Use::Use(User *_user, std::string _purpose)
    : user(user), uses(1), purposes({_purpose}) {}

User *Use::getUser() { return user; }

bool Use::isUser(User *_user) const { return (user == _user); }

unsigned Use::getUses() const { return uses; }

void Use::addUse(std::string purpose) {
  uses++;
  purposes.push_back(purpose);
}

std::vector<std::string> Use::getPurposes() { return purposes; }

Usable::Usable() {}

unsigned Usable::getUses(User *user) const {
  for (auto use : data) {
    if (use->isUser(user)) {
      return use->getUses();
    }
  }
  return 0;
}

void Usable::addUse(User *user, std::string purpose) {
  for (auto use : data) {
    if (use->isUser(user)) {
      use->addUse(purpose);
      return;
    }
  }
  data.push_back(new Use(user, purpose));
}

void Usable::removeUser(User *user) {
  for (auto use : data) {
    if (use->isUser(user)) {
      data.remove(use);
      return;
    }
  }
}

unsigned Usable::getTotalUses() const {
  unsigned result = 0;
  for (auto use : data) {
    result += use->getUses();
  }
  return result;
}

bool Usable::hasUsers() const { return (data.size() > 0); }

User::User() {}

bool User::hasUsable(Usable *other) const {
  for (auto use : uses) {
    if (use == other) {
      return true;
    }
  }
  return false;
}

void User::addUsable(Usable *other) { uses.push_back(other); }

void User::dropAllUses() {
  for (auto use : uses) {
    use->removeUser(this);
  }
}

} // namespace qat::IR