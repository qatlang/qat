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
	OperatorKind          opr;
	Vec<Argument*>        arguments;
	Type*                 returnType;
	Maybe<VisibilitySpec> visibSpec;
	Maybe<Identifier>     argName;
	FileRangePtr          nameRange;
	FileRangePtr          fileRange;

	PrerunExpression* defineChecker;
	Maybe<MetaInfo>   metaInfo;

  public:
	OperatorPrototype(bool _isVariationFn, OperatorKind _op, FileRangePtr _nameRange, Vec<Argument*> _arguments,
	                  Type* _returnType, Maybe<VisibilitySpec> _visibSpec, FileRangePtr _fileRange,
	                  Maybe<Identifier> _argName, PrerunExpression* _defineChecker, Maybe<MetaInfo> _metaInfo)
	    : isVariationFn(_isVariationFn), opr(_op), arguments(_arguments), returnType(_returnType),
	      visibSpec(_visibSpec), argName(_argName), nameRange(_nameRange), fileRange(_fileRange),
	      defineChecker(_defineChecker), metaInfo(std::move(_metaInfo)) {}

	static OperatorPrototype* create(bool _isVariationFn, OperatorKind _op, FileRangePtr _nameRange,
	                                 Vec<Argument*> _arguments, Type* _returnType, Maybe<VisibilitySpec> _visibSpec,
	                                 FileRangePtr _fileRange, Maybe<Identifier> _argName,
	                                 PrerunExpression* _defineChecker, Maybe<MetaInfo> _metaInfo) {
		return std::construct_at(OwnNormal(OperatorPrototype), _isVariationFn, _op, _nameRange, _arguments, _returnType,
		                         _visibSpec, _fileRange, _argName, _defineChecker, std::move(_metaInfo));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) {
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

	void define(MethodState& state, ir::Ctx* irCtx);

	NodeType nodeType() const { return NodeType::OPERATOR_PROTOTYPE; }

	~OperatorPrototype();
};

class OperatorDefinition {
	friend DefineStructType;
	friend DoSkill;

	Vec<Sentence*>     sentences;
	OperatorPrototype* prototype;
	FileRangePtr       fileRange;

  public:
	OperatorDefinition(OperatorPrototype* _prototype, Vec<Sentence*> _sentences, FileRangePtr _fileRange)
	    : sentences(_sentences), prototype(_prototype), fileRange(_fileRange) {}

	static OperatorDefinition* create(OperatorPrototype* _prototype, Vec<Sentence*> _sentences,
	                                  FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(OperatorDefinition), _prototype, _sentences, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) {
		for (auto snt : sentences) {
			UPDATE_DEPS(snt);
		}
	}

	void define(MethodState& state, ir::Ctx* irCtx);

	ir::Value* emit(MethodState& state, ir::Ctx* irCtx);

	NodeType nodeType() const { return NodeType::OPERATOR_DEFINITION; }
};

} // namespace qat::ast

#endif
