#ifndef QAT_AST_OPERATOR_FUNCTION_HPP
#define QAT_AST_OPERATOR_FUNCTION_HPP

#include "../IR/context.hpp"
#include "../IR/types/struct_type.hpp"
#include "./argument.hpp"
#include "./expressions/operator.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "member_parent_like.hpp"
#include "meta_info.hpp"

namespace qat::ast {

class OperatorPrototype {
  friend class OperatorDefinition;

private:
  bool                  isVariationFn;
  Op                    opr;
  Vec<Argument*>        arguments;
  Type*                 returnType;
  Maybe<VisibilitySpec> visibSpec;
  Maybe<Identifier>     argName;
  FileRange             nameRange;
  FileRange             fileRange;

  PrerunExpression* defineChecker;
  Maybe<MetaInfo>   metaInfo;

public:
  OperatorPrototype(bool _isVariationFn, Op _op, FileRange _nameRange, Vec<Argument*> _arguments, Type* _returnType,
                    Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange, Maybe<Identifier> _argName,
                    PrerunExpression* _defineChecker, Maybe<MetaInfo> _metaInfo)
      : isVariationFn(_isVariationFn), opr(_op), arguments(_arguments), returnType(_returnType), visibSpec(_visibSpec),
        argName(_argName), nameRange(_nameRange), fileRange(_fileRange), defineChecker(_defineChecker),
        metaInfo(std::move(_metaInfo)) {}

  useit static OperatorPrototype* create(bool _isVariationFn, Op _op, FileRange _nameRange, Vec<Argument*> _arguments,
                                         Type* _returnType, Maybe<VisibilitySpec> _visibSpec,
                                         const FileRange& _fileRange, Maybe<Identifier> _argName,
                                         PrerunExpression* _defineChecker, Maybe<MetaInfo> _metaInfo) {
    return std::construct_at(OwnNormal(OperatorPrototype), _isVariationFn, _op, _nameRange, _arguments, _returnType,
                             _visibSpec, _fileRange, _argName, _defineChecker, std::move(_metaInfo));
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
    if (defineChecker) {
      UPDATE_DEPS(defineChecker);
    }
    if (returnType) {
      UPDATE_DEPS(returnType);
    }
    for (auto arg : arguments) {
      if (arg->get_type()) {
        UPDATE_DEPS(arg->get_type());
      }
    }
  }

  void           define(MethodState& state, ir::Ctx* irCtx);
  useit Json     to_json() const;
  useit NodeType nodeType() const { return NodeType::OPERATOR_PROTOTYPE; }
  ~OperatorPrototype();
};

class OperatorDefinition {
  friend DefineCoreType;
  friend DoSkill;

  Vec<Sentence*>     sentences;
  OperatorPrototype* prototype;
  FileRange          fileRange;

public:
  OperatorDefinition(OperatorPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
      : sentences(_sentences), prototype(_prototype), fileRange(_fileRange) {}

  useit static OperatorDefinition* create(OperatorPrototype* _prototype, Vec<Sentence*> _sentences,
                                          FileRange _fileRange) {
    return std::construct_at(OwnNormal(OperatorDefinition), _prototype, _sentences, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
    for (auto snt : sentences) {
      UPDATE_DEPS(snt);
    }
  }

  void  define(MethodState& state, ir::Ctx* irCtx);
  useit ir::Value* emit(MethodState& state, ir::Ctx* irCtx);
  useit Json       to_json() const;
  useit NodeType   nodeType() const { return NodeType::OPERATOR_DEFINITION; }
};

} // namespace qat::ast

#endif
