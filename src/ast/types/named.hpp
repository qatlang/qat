#ifndef QAT_AST_TYPES_NAMED_HPP
#define QAT_AST_TYPES_NAMED_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class NamedType final : public Type {
  private:
	u32				relative;
	Vec<Identifier> names;

	Maybe<usize> typeSize;

  public:
	NamedType(u32 _relative, Vec<Identifier> _names, FileRange _fileRange)
		: Type(_fileRange), relative(_relative), names(_names) {}

	useit static NamedType* create(u32 _relative, Vec<Identifier> _names, FileRange _fileRange) {
		return std::construct_at(OwnNormal(NamedType), _relative, _names, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
							 EmitCtx* ctx) final;

	useit Maybe<usize> getTypeSizeInBits(EmitCtx* ctx) const final { return typeSize; }
	useit String	   get_name() const;
	useit u32		   getRelative() const;
	useit ir::Type*	  emit(EmitCtx* ctx) final;
	useit AstTypeKind type_kind() const final;
	useit Json		  to_json() const final;
	useit String	  to_string() const final;
};

} // namespace qat::ast

#endif
