#ifndef QAT_AST_KEY_VALUE_HPP
#define QAT_AST_KEY_VALUE_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"
#include "../utils/identifier.hpp"
#include "../utils/macros.hpp"

namespace qat::ast {

template <typename ValueTy> class KeyValue {
public:
  KeyValue(Identifier _key, ValueTy _value, FileRange _valueRange)
      : key(std::move(_key)), value(std::move(_value)), valueRange(std::move(_valueRange)) {}

  Identifier key;

  ValueTy   value;
  FileRange valueRange;

  operator JsonValue() {
    return Json()._("key", key.value)._("keyRange", key.range)._("value", value)._("valueRange", valueRange);
  }

  useit FileRange getRange() const { return {key.range, valueRange}; }
};

} // namespace qat::ast

#endif