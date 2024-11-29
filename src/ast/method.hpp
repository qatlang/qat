#ifndef QAT_AST_METHOD_HPP
#define QAT_AST_METHOD_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./expression.hpp"
#include "./member_parent_like.hpp"
#include "./meta_info.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

enum class MethodType {
  Static,
  normal,
  variation,
  valued,
};

inline String method_type_to_string(MethodType ty) {
  switch (ty) {
    case MethodType::Static:
      return "static";
    case MethodType::normal:
      return "normal";
    case MethodType::variation:
      return "variation";
    case MethodType::valued:
      return "valued";
  }
}

class MethodPrototype {
  friend class MethodDefinition;

  MethodType            fnTy;
  Identifier            name;
  Vec<Argument*>        arguments;
  bool                  isVariadic;
  Maybe<Type*>          returnType;
  Maybe<VisibilitySpec> visibSpec;
  FileRange             fileRange;

  PrerunExpression* defineChecker;
  Maybe<MetaInfo>   metaInfo;

public:
  MethodPrototype(MethodType _fnTy, Identifier _name, PrerunExpression* _defineChecker, Vec<Argument*> _arguments,
                  bool _isVariadic, Maybe<Type*> _returnType, Maybe<MetaInfo> _metaInfo,
                  Maybe<VisibilitySpec> visibSpec, FileRange _fileRange);

  static MethodPrototype* Normal(bool _isVariationFn, const Identifier& _name, PrerunExpression* _condition,
                                 const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                 Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> _visibSpec,
                                 const FileRange& _fileRange);

  static MethodPrototype* Static(const Identifier& _name, PrerunExpression* _condition,
                                 const Vec<Argument*>& _arguments, bool _isVariadic, Maybe<Type*> _returnType,
                                 Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> _visibSpec,
                                 const FileRange& _fileRange);

  static MethodPrototype* Value(const Identifier& _name, PrerunExpression* _condition, const Vec<Argument*>& _arguments,
                                bool _isVariadic, Maybe<Type*> _returnType, Maybe<MetaInfo> _metaInfo,
                                Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange);

  ir::EntityChildType fn_type_to_child_type() {
    switch (fnTy) {
      case MethodType::Static:
        return ir::EntityChildType::staticFn;
      case MethodType::normal:
        return ir::EntityChildType::method;
      case MethodType::variation:
        return ir::EntityChildType::variation;
      case MethodType::valued:
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
    if (defineChecker) {
      UPDATE_DEPS(defineChecker);
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
  ~MethodPrototype();
};

class MethodDefinition {
  friend class DefineCoreType;
  friend class DoSkill;

private:
  Vec<Sentence*>   sentences;
  MethodPrototype* prototype;
  FileRange        fileRange;

public:
  MethodDefinition(MethodPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
      : sentences(_sentences), prototype(_prototype), fileRange(_fileRange) {}

  useit static inline MethodDefinition* create(MethodPrototype* _prototype, Vec<Sentence*> _sentences,
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
