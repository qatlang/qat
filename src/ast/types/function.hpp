#ifndef QAT_AST_TYPES_FUNCTION_TYPE_HPP
#define QAT_AST_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class FunctionType final : public Type {
  private:
	Type*	   returnType;
	Vec<Type*> argTypes;
	bool	   isVariadic;

  public:
	FunctionType(Type* _retType, Vec<Type*> _argTypes, bool _isVariadic, FileRange _fileRange)
		: Type(_fileRange), returnType(_retType), argTypes(_argTypes), isVariadic(_isVariadic) {}

	useit static FunctionType* create(Type* _retType, Vec<Type*> _argTypes, bool _isVariadic, FileRange _fileRange) {
		return std::construct_at(OwnNormal(FunctionType), _retType, _argTypes, _isVariadic, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
							 EmitCtx* ctx) final;

	useit ir::Type*	  emit(EmitCtx* ctx) final;
	useit AstTypeKind type_kind() const final { return AstTypeKind::FUNCTION; }
	useit Json		  to_json() const final;
	useit String	  to_string() const final;
};

} // namespace qat::ast

#endif
