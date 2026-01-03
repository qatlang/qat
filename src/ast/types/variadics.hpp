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

	useit String to_string() const {
		switch (kind) {
			case VariadicKind::NORMAL:
				return "variadic";
			case VariadicKind::LEGACY:
				return "variadic:legacy";
			case VariadicKind::TYPED:
				return "variadic :: " + type->to_string();
		}
	}

	useit Json to_json() const {
		return Json()
		    ._("kind", kind == VariadicKind::NORMAL ? "normal" : (kind == VariadicKind::LEGACY ? "legacy" : "typed"))
		    ._("type", kind == VariadicKind::TYPED ? type->to_json() : JsonValue());
	}

	useit ir::Variadics to_ir(EmitCtx* irCtx) const;
};

} // namespace qat::ast

#endif
