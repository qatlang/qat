#ifndef QAT_AST_TYPES_VARIADICS_HPP
#define QAT_AST_TYPES_VARIADICS_HPP

#include "./qat_type.hpp"

namespace qat::ast {

enum class VariadicKind {
	NORMAL,
	LEGACY,
	TYPED,
};

struct Variadics {
	VariadicKind kind;
	Type*        type;
	FileRangePtr range;

	String to_string() const {
		switch (kind) {
			case VariadicKind::NORMAL:
				return "variadic";
			case VariadicKind::LEGACY:
				return "variadic:legacy";
			case VariadicKind::TYPED:
				return "variadic :: " + type->to_string();
		}
	}

	ir::Variadics to_ir(EmitCtx* irCtx) const;
};

} // namespace qat::ast

#endif
