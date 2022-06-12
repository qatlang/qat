#include "./box.hpp"

std::vector<std::string> qat::AST::Box::resolve() {
  std::vector<std::string> value;
  if (has_parent()) {
    value = parent->resolve();
  }
  value.push_back(name);
  return value;
}

std::string qat::AST::Box::generate() {
  std::vector<std::string> resolved = resolve();
  std::string result;
  if ((resolved.size() == 1) && (resolved.at(0) == "")) {
    return result;
  } else {
    for (auto &value : resolved) {
      result += value;
      result += ":";
    }
    return result;
  }
}

void qat::AST::Box::close() { isOpen = false; }

bool qat::AST::Box::has_parent() { return parent; }
