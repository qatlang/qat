#ifndef QAT_AST_EXPRESSIONS_DEFAULT_HPP
#define QAT_AST_EXPRESSIONS_DEFAULT_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class Default final : public Expression, public LocalDeclCompatible, public TypeInferrable {
	friend class LocalDeclaration;
	friend class Assignment;

  private:
	Maybe<ast::Type*> providedType;

  public:
	Default(Maybe<ast::Type*> _providedType, FileRange _fileRange)
	    : Expression(std::move(_fileRange)), providedType(_providedType) {}

	useit static Default* create(Maybe<ast::Type*> _providedType, FileRange _fileRange) {
		return std::construct_at(OwnNormal(Default), _providedType, _fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		if (providedType.has_value()) {
			providedType.value()->update_dependencies(phase, ir::DependType::childrenPartial, ent, ctx);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) final;
	useit NodeType   nodeType() const final { return NodeType::DEFAULT; }
	useit Json       to_json() const final;
};

} // namespace qat::ast

#endif
