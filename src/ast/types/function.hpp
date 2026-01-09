#ifndef QAT_AST_TYPES_FUNCTION_TYPE_HPP
#define QAT_AST_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"
#include "./variadics.hpp"

namespace qat::ast {

class FunctionType final : public Type {
  private:
	Type*            returnType;
	Vec<Type*>       argTypes;
	Maybe<Variadics> variadics;

  public:
	FunctionType(Type* _retType, Vec<Type*> _argTypes, Maybe<Variadics> _variadics, FileRangePtr _fileRange)
	    : Type(_fileRange), returnType(_retType), argTypes(_argTypes), variadics(_variadics) {}

	static FunctionType* create(Type* retType, Vec<Type*> argTypes, Maybe<Variadics> variadics,
	                            FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(FunctionType), retType, argTypes, variadics, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	ir::Type* emit(EmitCtx* ctx) final;

	AstTypeKind type_kind() const final { return AstTypeKind::FUNCTION; }

	String to_string() const final;
};

} // namespace qat::ast

#endif
