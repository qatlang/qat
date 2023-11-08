#ifndef QAT_AST_TYPES_RESULT_HPP
#define QAT_AST_TYPES_RESULT_HPP

#include "qat_type.hpp"
namespace qat::ast {
class ResultType : public QatType {
  ast::QatType* validType;
  ast::QatType* errorType;
  bool          isPacked;

public:
  ResultType(ast::QatType* validType, ast::QatType* errorType, bool isPacked, FileRange fileRange);

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit Json         toJson() const final;
  useit TypeKind     typeKind() const final;
  useit String       toString() const final;
};
} // namespace qat::ast

#endif