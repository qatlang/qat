#ifndef QAT_IR_MENTION_HPP
#define QAT_IR_MENTION_HPP

#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"

namespace qat::IR {

class Mention {
public:
  Mention(String _kind, FileRange _origin, FileRange _source)
      : kind(std::move(_kind)), origin(std::move(_origin)), source(std::move(_source)) {}

  String    kind;
  FileRange origin;
  FileRange source;
};

} // namespace qat::IR

#endif