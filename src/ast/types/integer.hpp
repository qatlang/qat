#ifndef QAT_AST_TYPES_INTEGER_HPP
#define QAT_AST_TYPES_INTEGER_HPP

#include "../../IR/context.hpp"
#include "qat_type.hpp"

namespace qat::ast {

class IntegerType final : public Type {
	friend class BringBitwidths;
	friend class FillGeneric;

  private:
	const u32 bitWidth;

	mutable bool isPartOfGeneric = false;

  public:
	IntegerType(u32 _bitWidth, FileRange _fileRange) : Type(_fileRange), bitWidth(_bitWidth) {}

	useit static IntegerType* create(u32 _bitWidth, FileRange _fileRange) {
		return std::construct_at(OwnNormal(IntegerType), _bitWidth, _fileRange);
	}

	useit Maybe<usize> getTypeSizeInBits(EmitCtx* ctx) const final;

	ir::Type*	emit(EmitCtx* ctx);
	AstTypeKind type_kind() const;
	bool		isBitWidth(const u32 width) const;
	Json		to_json() const;
	String		to_string() const;
};

} // namespace qat::ast

#endif
