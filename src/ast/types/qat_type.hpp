#ifndef QAT_AST_TYPES_QAT_TYPE_HPP
#define QAT_AST_TYPES_QAT_TYPE_HPP

#include "../../IR/context.hpp"
#include "../../backend/cpp.hpp"
#include "../../utils/file_range.hpp"
#include "./type_kind.hpp"

#include <string>

namespace qat::ast {

/**
 *  This is the base class representing a type in the language
 *
 */
class QatType {
private:
  bool variable;

public:
  QatType(const bool _variable, const utils::FileRange _fileRange)
      : variable(_variable), fileRange(_fileRange) {}

  virtual ~QatType() {}

  // This is the code ctx function that handles the generation of
  // LLVM IR
  virtual IR::QatType *emit(IR::Context *ctx) {}

  // TypeKind is used to detect variants of the QatType
  virtual TypeKind typeKind() {}

  // Whether this type was accompanied by the var keyword, which
  // represents variability for the value in context
  bool isVariable() const { return variable; }

  bool isConstant() const { return !variable; }

  // This generates JSON to represent the type
  virtual nuo::Json toJson() const {}

  // FileRange representing the range in the file this type was
  // parsed from
  utils::FileRange fileRange;

  virtual std::string toString() const {}

  virtual void destroy() {}
};

} // namespace qat::ast

#endif