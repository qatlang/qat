#ifndef QAT_AST_TYPES_CHAR_HPP
#define QAT_AST_TYPES_CHAR_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class CharType final : public Type {

  public:
	explicit CharType(FileRangePtr _fileRange) : Type(_fileRange) {}

	static CharType* create(FileRangePtr fileRange) { return std::construct_at(OwnNormal(CharType), fileRange); }

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	Maybe<usize> get_type_bitsize(EmitCtx*) const final { return 21u; }

	ir::Type* emit(EmitCtx* ctx);

	AstTypeKind type_kind() const final { return AstTypeKind::CHAR; }

	String to_string() const { return "char"; }
};

} // namespace qat::ast

#endif
