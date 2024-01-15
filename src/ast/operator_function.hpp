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
  OperatorPrototype(bool _isVariationFn, Op _op, FileRange nameRange, Vec<Argument*> _arguments, QatType* _returnType,
                    Maybe<VisibilitySpec> visibSpec, const FileRange& _fileRange, Maybe<Identifier> _argName);

  void setMemberParent(IR::MemberParent* _memberParent) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::operatorPrototype; }
  ~OperatorPrototype() final;
};

class OperatorDefinition : public Node {
private:
  Vec<Sentence*>     sentences;
  OperatorPrototype* prototype;

public:
  OperatorDefinition(OperatorPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange);

  void setMemberParent(IR::MemberParent* coreType) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::operatorDefinition; }
};

} // namespace qat::ast

#endif
