#include "./use.hpp"
#include "../memory_tracker.hpp"
#include <algorithm>

namespace qat::IR {

Use::Use(User* _user, String _purpose) : user(_user), uses(1), purposes({std::move(_purpose)}) {}

User* Use::getUser() { return user; }

bool Use::isUser(User* _user) const { return (user == _user); }

u64 Use::getUses() const { return uses; }

void Use::addUse(const String& purpose) {
  uses++;
  purposes.push_back(purpose);
}

Vec<String> Use::getPurposes() { return purposes; }

Usable::Usable() = default;

u64 Usable::getUses(User* user) const {
  for (auto* use : data) {
    if (use->isUser(user)) {
      return use->getUses();
    }
  }
  return 0;
}

void Usable::addUse(User* user, const String& purpose) {
  for (auto* use : data) {
    if (use->isUser(user)) {
      use->addUse(purpose);
      return;
    }
  }
  data.push_back(new Use(user, purpose));
}

void Usable::removeUser(User* user) {
  for (auto* use : data) {
    if (use->isUser(user)) {
      data.remove(use);
      return;
    }
  }
}

u64 Usable::getTotalUses() const {
  u64 result = 0;
  for (auto* use : data) {
    result += use->getUses();
  }
  return result;
}

bool Usable::hasUsers() const { return !data.empty(); }

User::User() = default;

bool User::hasUsable(Usable* other) const {
  for (const auto& elem : uses) {
    if (elem == other) {
      return true;
    }
  }
  return false;
}

void User::addUsable(Usable* other) { uses.push_back(other); }

void User::dropAllUses() {
  for (auto* use : uses) {
    use->removeUser(this);
  }
}

} // namespace qat::IR