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
	friend class ir::GenericFunction;
	Identifier            name;
	Vec<Argument*>        arguments;
	Maybe<Type*>          returnType;
	Maybe<MetaInfo>       metaInfo;
	Maybe<VisibilitySpec> visibSpec;
	PrerunExpression*     defineChecker;
	PrerunExpression*     genericConstraint;

	Maybe<Pair<Vec<Sentence*>, FileRange>> definition;

	Vec<GenericAbstractType*> generics;
	ir::GenericFunction*      genericFn = nullptr;

	mutable Maybe<llvm::GlobalValue::LinkageTypes> linkageType;

	mutable Maybe<String> variantName;
	mutable ir::Function* function = nullptr;
	mutable bool          isMainFn = false;
	mutable Maybe<bool>   checkResult;

  public:
	FunctionPrototype(Identifier _name, Vec<Argument*> _arguments, bool _isVariadic, Maybe<Type*> _returnType,
	                  PrerunExpression* _checker, PrerunExpression* _genericConstraint, Maybe<MetaInfo> _metaInfo,
	                  Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange,
	                  Vec<GenericAbstractType*> _generics, Maybe<Pair<Vec<Sentence*>, FileRange>> _definition)
	    : IsEntity(_fileRange), name(_name), arguments(_arguments), returnType(_returnType), metaInfo(_metaInfo),
	      visibSpec(_visibSpec), defineChecker(_checker), genericConstraint(_genericConstraint),
	      definition(_definition), generics(_generics) {}

	useit static FunctionPrototype* create(Identifier _name, Vec<Argument*> _arguments, bool _isVariadic,
	                                       Maybe<Type*> _returnType, PrerunExpression* _checker,
	                                       PrerunExpression* _genericConstraint, Maybe<MetaInfo> _metaInfo,
	                                       Maybe<VisibilitySpec> _visibSpec, const FileRange& _fileRange,
	                                       Vec<GenericAbstractType*>              _generics,
	                                       Maybe<Pair<Vec<Sentence*>, FileRange>> _definition) {
		return std::construct_at(OwnNormal(FunctionPrototype), _name, _arguments, _isVariadic, _returnType, _checker,
		                         _genericConstraint, _metaInfo, _visibSpec, _fileRange, _generics, _definition);
	}

	useit bool is_generic() const;
	useit Vec<GenericAbstractType*> get_generics() const;

	void set_variant_name(const String& value) const;
	void unset_variant_name() const;

	ir::Function* create_function(ir::Mod* mod, ir::Ctx* irCtx) const;

	void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;
	void update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) final;
	void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) final;

	void           emit_definition(ir::Mod* mod, ir::Ctx* irCtx);
	useit Json     to_json() const final;
	useit NodeType nodeType() const final { return NodeType::FUNCTION_PROTOTYPE; }
	~FunctionPrototype() final;
};

} // namespace qat::ast

#endif
