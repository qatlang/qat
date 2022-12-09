#ifndef QAT_UTILS_IDENTIFIER_HPP
#define QAT_UTILS_IDENTIFIER_HPP

#include "./helpers.hpp"
#include "file_range.hpp"

namespace qat {

class Identifier {
public:
  Identifier(String value, utils::FileRange range);

  String           value;
  utils::FileRange range;

  operator JsonValue() const;
};

} // namespace qat

#endif