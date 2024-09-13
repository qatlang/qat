#ifndef QAT_AST_MEMBER_FUNCTION_HPP
#define QAT_AST_MEMBER_FUNCTION_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "expression.hpp"
#include "member_parent_like.hpp"
#include "meta_info.hpp"
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

class MemberPrototype {
  friend class MethodDefinition;

  AstMemberFnType          fnTy;
  Identifier               name;
  Maybe<PrerunExpression*> condition;
  Vec<Argument*>           arguments;
  bool                     isVariadic;
  Maybe<Type*>             returnType;
  Maybe<MetaInfo>          metaInfo;
  Maybe<VisibilitySpec>    visibSpec;
  FileRange                fileRange;

public:
  MemberPrototype(AstMemberFnType _fnTy, Identifier _name, Maybe<PrerunExpression*> _condition,
                  Vec<Argument*> _arguments, bool _isVariadic, Maybe<Type*> _returnType, Maybe<MetaInfo> _metaInfo,
                  Maybe<VisibilitySpec> visibSpec, FileRange _fileRange);

  static MemberPrototype* Normal(bool _isVariationFn, const Identifier& _name, Maybe<PrerunExpression*> _condition,
                                 const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                 Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> _visibSpec,
                                 const FileRange& _fileRange);

  static MemberPrototype* Static(const Identifier& _name, Maybe<PrerunExpression*> _condition,
                                 const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                 Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> _visibSpec,
                                 const FileRange& _fileRange);

  static MemberPrototype* Value(const Identifier& _name, Maybe<PrerunExpression*> _condition,
                                const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> _visibSpec,
                                const FileRange& _fileRange);

  ir::EntityChildType fn_type_to_child_type() {
    switch (fnTy) {
      case AstMemberFnType::Static:
        return ir::EntityChildType::staticFn;
      case AstMemberFnType::normal:
        return ir::EntityChildType::method;
      case AstMemberFnType::variation:
        return ir::EntityChildType::variation;
      case AstMemberFnType::valued:
        return ir::EntityChildType::valued;
    }
  }

  void add_to_parent(ir::EntityState* ent, ir::Ctx* irCtx) {
    if (ent->has_child(name.value)) {
      auto ch = ent->get_child(name.value);
      irCtx->Error("Found " + ir::entity_child_type_to_string(ch.first) + " named " + irCtx->color(name.value) +
                       " found",
                   name.range);
    }
    ent->add_child({fn_type_to_child_type(), name.value});
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
    if (condition.has_value()) {
      UPDATE_DEPS(condition.value());
    }
    if (returnType.has_value()) {
      UPDATE_DEPS(returnType.value());
    }
    for (auto arg : arguments) {
      if (arg->get_type()) {
        UPDATE_DEPS(arg->get_type());
      }
    }
  }

  void           define(MethodState& state, ir::Ctx* irCtx);
  useit Json     to_json() const;
  useit NodeType nodeType() const { return NodeType::MEMBER_PROTOTYPE; }
  ~MemberPrototype();
};

class MethodDefinition {
  friend class DefineCoreType;
  friend class DoSkill;

private:
  Vec<Sentence*>   sentences;
  MemberPrototype* prototype;
  FileRange        fileRange;

public:
  MethodDefinition(MemberPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
      : sentences(_sentences), prototype(_prototype), fileRange(_fileRange) {}

  useit static inline MethodDefinition* create(MemberPrototype* _prototype, Vec<Sentence*> _sentences,
                                               FileRange _fileRange) {
    return std::construct_at(OwnNormal(MethodDefinition), _prototype, _sentences, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
  }

  void  define(MethodState& state, ir::Ctx* irCtx);
  useit ir::Value* emit(MethodState& state, ir::Ctx* irCtx);
  useit Json       to_json() const;
  useit NodeType   nodeType() const { return NodeType::MEMBER_DEFINITION; }
};

} // namespace qat::ast

#endif
