#ifndef QAT_AST_ARGUMENT_HPP
#define QAT_AST_ARGUMENT_HPP

#include "../utils/file_range.hpp"
#include "../utils/macros.hpp"
#include "./types/qat_type.hpp"
#include <optional>
#include <string>

namespace qat::ast {

class Argument {
private:
  String name;

  utils::FileRange fileRange;

  QatType *type;

  bool isMember;

  Argument(String _name, utils::FileRange _fileRange, QatType *_type,
           bool _isMember);

public:
  static Argument *Normal(String name, utils::FileRange fileRange,
                          QatType *type);

  static Argument *ForConstructor(String name, utils::FileRange fileRange,
                                  QatType *type, bool isMember);

  useit String getName() const;

  useit auto getFileRange() -> utils::FileRange const;

  auto getType() -> QatType *;

  useit bool isTypeMember() const;
};

} // namespace qat::ast

#endif