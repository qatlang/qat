#ifndef QAT_AST_KEY_VALUE_HPP
#define QAT_AST_KEY_VALUE_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"

namespace qat::ast {

template <typename ValueTy> class KeyValue {
public:
  KeyValue(String _key, utils::FileRange _keyRange, ValueTy _value, utils::FileRange _valueRange)
      : key(std::move(_key)), keyRange(std::move(_keyRange)), value(std::move(_value)),
        valueRange(std::move(_valueRange)) {}

  String           key;
  utils::FileRange keyRange;

  ValueTy          value;
  utils::FileRange valueRange;

  useit utils::FileRange getRange() const { return {keyRange, valueRange}; }
};

} // namespace qat::ast

#endif