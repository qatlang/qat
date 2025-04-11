#ifndef QAT_AST_TYPES_CHAR_HPP
#define QAT_AST_TYPES_CHAR_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class CharType final : public Type {

  public:
	explicit CharType(FileRange _fileRange) : Type(std::move(_fileRange)) {}

	useit static CharType* create(FileRange fileRange) {
		return std::construct_at(OwnNormal(CharType), std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final {}

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final { return 21u; }

	useit ir::Type* emit(EmitCtx* ctx);

	useit AstTypeKind type_kind() const final { return AstTypeKind::CHAR; }

	useit Json to_json() const { return Json()._("typeKind", "char")._("fileRange", fileRange); }

	useit String to_string() const { return "char"; }
};

} // namespace qat::ast

#endif
