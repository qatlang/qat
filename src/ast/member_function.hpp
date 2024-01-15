#ifndef QAT_AST_MEMBER_FUNCTION_HPP
#define QAT_AST_MEMBER_FUNCTION_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class MemberPrototype : public Node {
  friend class MemberDefinition;

private:
  bool                  isVariationFn;
  Identifier            name;
  Vec<Argument*>        arguments;
  bool                  isVariadic;
  QatType*              returnType;
  Maybe<VisibilitySpec> visibSpec;
  bool                  isStatic;

  mutable IR::MemberParent*   memberParent = nullptr;
  mutable IR::MemberFunction* memberFn     = nullptr;

  MemberPrototype(bool isStatic, bool _isVariationFn, Identifier _name, Vec<Argument*> _arguments, bool _isVariadic,
                  QatType* _returnType, Maybe<VisibilitySpec> visibSpec, FileRange _fileRange);

public:
  static MemberPrototype* Normal(bool _isVariationFn, const Identifier& _name, const Vec<Argument*>& _arguments,
                                 bool _isVariadic, QatType* _returnType, Maybe<VisibilitySpec> _visibSpec,
                                 const FileRange& _fileRange);

  static MemberPrototype* Static(const Identifier& _name, const Vec<Argument*>& _arguments, bool _isVariadic,
                                 QatType* _returnType, Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange);

  void setMemberParent(IR::MemberParent* _memPar) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::memberPrototype; }
  ~MemberPrototype();
};

class MemberDefinition : public Node {
private:
  Vec<Sentence*>   sentences;
  MemberPrototype* prototype;

public:
  MemberDefinition(MemberPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange);

  void setMemberParent(IR::MemberParent* coreType) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::memberDefinition; }
};

} // namespace qat::ast

#endif
