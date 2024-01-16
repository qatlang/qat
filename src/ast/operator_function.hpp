#ifndef QAT_AST_OPERATOR_FUNCTION_HPP
#define QAT_AST_OPERATOR_FUNCTION_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./expressions/operator.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class OperatorPrototype final : public Node {
  friend class OperatorDefinition;

private:
  bool                  isVariationFn;
  Op                    opr;
  Vec<Argument*>        arguments;
  QatType*              returnType;
  Maybe<VisibilitySpec> visibSpec;
  Maybe<Identifier>     argName;
  FileRange             nameRange;

  mutable IR::MemberParent*   memberParent = nullptr;
  mutable IR::MemberFunction* memberFn     = nullptr;

public:
  OperatorPrototype(bool _isVariationFn, Op _op, FileRange _nameRange, Vec<Argument*> _arguments, QatType* _returnType,
                    Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange, Maybe<Identifier> _argName)
      : Node(_fileRange), isVariationFn(_isVariationFn), opr(_op), arguments(_arguments), returnType(_returnType),
        visibSpec(_visibSpec), argName(_argName), nameRange(_nameRange) {}

  useit static inline OperatorPrototype* create(bool _isVariationFn, Op _op, FileRange _nameRange,
                                                Vec<Argument*> _arguments, QatType* _returnType,
                                                Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange,
                                                Maybe<Identifier> _argName) {
    return std::construct_at(OwnNormal(OperatorPrototype), _isVariationFn, _op, _nameRange, _arguments, _returnType,
                             _visibSpec, _fileRange, _argName);
  }

  void setMemberParent(IR::MemberParent* _memberParent) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::OPERATOR_PROTOTYPE; }
  ~OperatorPrototype() final;
};

class OperatorDefinition : public Node {
private:
  Vec<Sentence*>     sentences;
  OperatorPrototype* prototype;

public:
  OperatorDefinition(OperatorPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
      : Node(_fileRange), sentences(_sentences), prototype(_prototype) {}

  useit static inline OperatorDefinition* create(OperatorPrototype* _prototype, Vec<Sentence*> _sentences,
                                                 FileRange _fileRange) {
    return std::construct_at(OwnNormal(OperatorDefinition), _prototype, _sentences, _fileRange);
  }

  void setMemberParent(IR::MemberParent* coreType) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::OPERATOR_DEFINITION; }
};

} // namespace qat::ast

#endif
