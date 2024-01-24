#ifndef QAT_AST_TYPES_FUNCTION_TYPE_HPP
#define QAT_AST_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class FunctionType : public QatType {
private:
  QatType*      returnType;
  Vec<QatType*> argTypes;

public:
  FunctionType(QatType* _retType, Vec<QatType*> _argTypes, FileRange _fileRange)
      : QatType(_fileRange), returnType(_retType), argTypes(_argTypes) {}

  useit static inline FunctionType* create(QatType* _retType, Vec<QatType*> _argTypes, FileRange _fileRange) {
    return std::construct_at(OwnNormal(FunctionType), _retType, _argTypes, _fileRange);
  }

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit AstTypeKind  typeKind() const final { return AstTypeKind::FUNCTION; }
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif