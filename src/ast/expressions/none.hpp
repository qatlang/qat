#ifndef QAT_AST_NONE_HPP
#define QAT_AST_NONE_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class NoneExpression : public Expression, public TypeInferrable {
  friend class Assignment;
  friend class LocalDeclaration;
  QatType*         type = nullptr;
  Maybe<FileRange> isPacked;

public:
  NoneExpression(Maybe<FileRange> _isPacked, QatType* _type, FileRange _fileRange)
      : Expression(std::move(_fileRange)), type(_type), isPacked(_isPacked) {}

  useit static inline NoneExpression* create(Maybe<FileRange> isPacked, QatType* _type, FileRange _fileRange) {
    return std::construct_at(OwnNormal(NoneExpression), isPacked, _type, _fileRange);
  }

  useit inline bool hasTypeSet() const { return type != nullptr; }

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::NONE; }
};

} // namespace qat::ast

#endif