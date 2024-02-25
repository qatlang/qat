#ifndef QAT_AST_FUNCTION_HPP
#define QAT_AST_FUNCTION_HPP

#include "../IR/context.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./sentence.hpp"
#include "meta_info.hpp"
#include "types/generic_abstract.hpp"

namespace qat::ast {

class FunctionPrototype final : public IsEntity {
  friend class IR::GenericFunction;
  Identifier               name;
  Vec<Argument*>           arguments;
  bool                     isVariadic;
  Maybe<QatType*>          returnType;
  Maybe<MetaInfo>          metaInfo;
  Maybe<VisibilitySpec>    visibSpec;
  Maybe<PrerunExpression*> checker;
  Maybe<PrerunExpression*> genericConstraint;

  Maybe<Pair<Vec<Sentence*>, FileRange>> definition;

  Vec<GenericAbstractType*> generics;
  IR::GenericFunction*      genericFn = nullptr;

  mutable Maybe<llvm::GlobalValue::LinkageTypes> linkageType;

  mutable Maybe<String> variantName;
  mutable IR::Function* function = nullptr;
  mutable bool          isMainFn = false;
  mutable Maybe<bool>   checkResult;

public:
  FunctionPrototype(Identifier _name, Vec<Argument*> _arguments, bool _isVariadic, Maybe<QatType*> _returnType,
                    Maybe<PrerunExpression*> _checker, Maybe<PrerunExpression*> _genericConstraint,
                    Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange,
                    Vec<GenericAbstractType*> _generics, Maybe<Pair<Vec<Sentence*>, FileRange>> _definition)
      : IsEntity(_fileRange), name(_name), arguments(_arguments), isVariadic(_isVariadic), returnType(_returnType),
        metaInfo(_metaInfo), visibSpec(_visibSpec), checker(_checker), genericConstraint(_genericConstraint),
        generics(_generics), definition(_definition) {}

  useit static inline FunctionPrototype* create(Identifier _name, Vec<Argument*> _arguments, bool _isVariadic,
                                                Maybe<QatType*> _returnType, Maybe<PrerunExpression*> _checker,
                                                Maybe<PrerunExpression*> _genericConstraint, Maybe<MetaInfo> _metaInfo,
                                                Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange,
                                                Vec<GenericAbstractType*>              _generics,
                                                Maybe<Pair<Vec<Sentence*>, FileRange>> _definition) {
    return std::construct_at(OwnNormal(FunctionPrototype), _name, _arguments, _isVariadic, _returnType, _checker,
                             _genericConstraint, _metaInfo, _visibSpec, _fileRange, _generics, _definition);
  }

  useit bool isGeneric() const;
  useit Vec<GenericAbstractType*> getGenerics() const;
  void                            setVariantName(const String& value) const;
  void                            unsetVariantName() const;
  IR::Function*                   create_function(IR::QatModule* mod, IR::Context* ctx) const;

  void create_entity(IR::QatModule* parent, IR::Context* ctx) final;
  void update_entity_dependencies(IR::QatModule* mod, IR::Context* ctx) final;
  void do_phase(IR::EmitPhase phase, IR::QatModule* parent, IR::Context* ctx) final;

  useit void     emit_definition(IR::Context* ctx);
  useit Json     toJson() const final;
  useit NodeType nodeType() const final { return NodeType::FUNCTION_PROTOTYPE; }
  ~FunctionPrototype() final;
};

} // namespace qat::ast

#endif
