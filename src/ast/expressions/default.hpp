#ifndef QAT_AST_EXPRESSIONS_DEFAULT_HPP
#define QAT_AST_EXPRESSIONS_DEFAULT_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class Default : public Expression, public LocalDeclCompatible, public TypeInferrable {
  friend class LocalDeclaration;
  friend class Assignment;

private:
  Maybe<ast::QatType*> providedType;

public:
  Default(Maybe<ast::QatType*> _providedType, FileRange _fileRange)
      : Expression(std::move(_fileRange)), providedType(_providedType) {}

  useit static inline Default* create(Maybe<ast::QatType*> _providedType, FileRange _fileRange) {
    return std::construct_at(OwnNormal(Default), _providedType, _fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::DEFAULT; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif