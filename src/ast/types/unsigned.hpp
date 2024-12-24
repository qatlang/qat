#ifndef QAT_AST_TYPES_UNSIGNED_HPP
#define QAT_AST_TYPES_UNSIGNED_HPP

#include "../../IR/context.hpp"
#include "qat_type.hpp"

namespace qat::ast {

class UnsignedType final : public Type {
	friend class BringBitwidths;
	friend class FillGeneric;

  private:
	u32  bitWidth;
	bool is_bool;

	mutable bool isPartOfGeneric = false;

  public:
	UnsignedType(u64 _bitWidth, bool _isBool, FileRange _fileRange)
	    : Type(_fileRange), bitWidth(_bitWidth), is_bool(_isBool) {}

	useit static UnsignedType* create(u64 _bitWidth, bool _isBool, FileRange _fileRange) {
		return std::construct_at(OwnNormal(UnsignedType), _bitWidth, _isBool, _fileRange);
	}

	useit Maybe<usize> get_type_bitsize(EmitCtx* ctx) const final;

	useit ir::Type*   emit(EmitCtx* ctx);
	useit AstTypeKind type_kind() const final;
	useit bool        isBitWidth(u32 width) const;
	useit Json        to_json() const final;
	useit String      to_string() const final;
};

} // namespace qat::ast

#endif
