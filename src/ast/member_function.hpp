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

enum class AstMemberFnType {
  Static,
  normal,
  variation,
  valued,
};

inline String member_fn_type_to_string(AstMemberFnType ty) {
  switch (ty) {
    case AstMemberFnType::Static:
      return "static";
    case AstMemberFnType::normal:
      return "normal";
    case AstMemberFnType::variation:
      return "variation";
    case AstMemberFnType::valued:
      return "valued";
  }
}

class MemberPrototype : public Node {
  friend class MemberDefinition;

private:
  AstMemberFnType       fnTy;
  Identifier            name;
  Vec<Argument*>        arguments;
  bool                  isVariadic;
  Maybe<QatType*>       returnType;
  Maybe<VisibilitySpec> visibSpec;

  mutable IR::MemberParent*   memberParent = nullptr;
  mutable IR::MemberFunction* memberFn     = nullptr;

  MemberPrototype(AstMemberFnType _fnTy, Identifier _name, Vec<Argument*> _arguments, bool _isVariadic,
                  Maybe<QatType*> _returnType, Maybe<VisibilitySpec> visibSpec, FileRange _fileRange);

public:
  static MemberPrototype* Normal(bool _isVariationFn, const Identifier& _name, const Vec<Argument*>& _arguments,
                                 bool _isVariadic, Maybe<QatType*> _returnType, Maybe<VisibilitySpec> _visibSpec,
                                 const FileRange& _fileRange);

  static MemberPrototype* Static(const Identifier& _name, const Vec<Argument*>& _arguments, bool _isVariadic,
                                 Maybe<QatType*> _returnType, Maybe<VisibilitySpec> _visibSpec,
                                 const FileRange& _fileRange);

  static MemberPrototype* Value(const Identifier& _name, const Vec<Argument*>& _arguments, bool _isVariadic,
                                Maybe<QatType*> _returnType, Maybe<VisibilitySpec> _visibSpec,
                                const FileRange& _fileRange);

  void setMemberParent(IR::MemberParent* _memPar) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::MEMBER_PROTOTYPE; }
  ~MemberPrototype();
};

class MemberDefinition : public Node {
private:
  Vec<Sentence*>   sentences;
  MemberPrototype* prototype;

public:
  MemberDefinition(MemberPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
      : Node(_fileRange), sentences(_sentences), prototype(_prototype) {}

  useit static inline MemberDefinition* create(MemberPrototype* _prototype, Vec<Sentence*> _sentences,
                                               FileRange _fileRange) {
    return std::construct_at(OwnNormal(MemberDefinition), _prototype, _sentences, _fileRange);
  }

  void setMemberParent(IR::MemberParent* coreType) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::MEMBER_DEFINITION; }
};

} // namespace qat::ast

#endif
