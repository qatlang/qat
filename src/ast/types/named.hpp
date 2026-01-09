#ifndef QAT_AST_TYPES_NAMED_HPP
#define QAT_AST_TYPES_NAMED_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class NamedType final : public Type {
  private:
	u32             relative;
	Vec<Identifier> names;

	Maybe<usize> typeSize;

  public:
	NamedType(u32 _relative, Vec<Identifier> _names, FileRangePtr _fileRange)
	    : Type(_fileRange), relative(_relative), names(_names) {}

	static NamedType* create(u32 _relative, Vec<Identifier> _names, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(NamedType), _relative, _names, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                         EmitCtx* ctx) final;

	Maybe<usize> get_type_bitsize(EmitCtx*) const final { return typeSize; }

	String get_name() const;

	u32 getRelative() const;

	ir::Type* emit(EmitCtx* ctx) final;

	AstTypeKind type_kind() const final;

	String to_string() const final;
};

} // namespace qat::ast

#endif
