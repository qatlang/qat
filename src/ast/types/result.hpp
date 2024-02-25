#ifndef QAT_AST_TYPES_RESULT_HPP
#define QAT_AST_TYPES_RESULT_HPP

#include "qat_type.hpp"
namespace qat::ast {

class ResultType final : public QatType {
  ast::QatType* validType;
  ast::QatType* errorType;
  bool          isPacked;

public:
  ResultType(ast::QatType* _validType, ast::QatType* _errorType, bool _isPacked, FileRange _fileRange)
      : QatType(_fileRange), validType(_validType), errorType(_errorType), isPacked(_isPacked) {}

  useit static inline ResultType* create(ast::QatType* _validType, ast::QatType* _errorType, bool _isPacked,
                                         FileRange _fileRange) {
    return std::construct_at(OwnNormal(ResultType), _validType, _errorType, _isPacked, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> expect, IR::EntityState* ent,
                           IR::Context* ctx) final;

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit Json         toJson() const final;
  useit AstTypeKind  typeKind() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif