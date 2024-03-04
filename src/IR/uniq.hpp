#ifndef QAT_IR_ELEMENT_HPP
#define QAT_IR_ELEMENT_HPP

#include "../utils/macros.hpp"
#include "../utils/unique_id.hpp"
#include <string>

namespace qat::ir {

class Uniq {
private:
  String id;

public:
  explicit Uniq(String idVal) : id(std::move(idVal)) {}
  Uniq() : id(utils::unique_id()) {}

  useit String get_id() const { return id; }
  useit bool   isID(const String& val) const { return (val == id); }
};

} // namespace qat::ir

#endif