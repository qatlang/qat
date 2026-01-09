#ifndef QAT_AST_GENERICS_HPP
#define QAT_AST_GENERICS_HPP

#include "./types/qat_type.hpp"
#include "expression.hpp"
#include "node.hpp"

namespace qat::ast {

enum class FillGenericKind {
	typed,
	prerun,
};

class FillGeneric {
	void*           data;
	FillGenericKind kind;

  public:
	explicit FillGeneric(Type* type) : data(type), kind(FillGenericKind::typed) {}

	explicit FillGeneric(PrerunExpression* expression) : data(expression), kind(FillGenericKind::prerun) {}

	static FillGeneric* create(Type* _type) { return std::construct_at(OwnNormal(FillGeneric), _type); }

	static FillGeneric* create(PrerunExpression* _exp) { return std::construct_at(OwnNormal(FillGeneric), _exp); }

	bool is_type() const;
	bool is_prerun() const;

	Type*             as_type() const;
	PrerunExpression* as_prerun() const;

	FileRangePtr get_range() const;

	ir::GenericToFill* toFill(EmitCtx* ctx) const;

	String to_string() const;
};

} // namespace qat::ast

#endif
