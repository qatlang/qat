#ifndef QAT_IR_TYPES_CHAR_HPP
#define QAT_IR_TYPES_CHAR_HPP

#include "../../utils/qat_region.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class CharType final : public Type {
	static CharType* charType;

  public:
	CharType(llvm::LLVMContext& llctx);

	static CharType* get(llvm::LLVMContext& llctx) {
		if (charType) {
			return charType;
		}
		return std::construct_at(OwnNormal(CharType), llctx);
	}

	bool can_be_prerun() const final { return true; }

	bool can_be_prerun_generic() const final { return true; }

	Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	bool is_type_sized() const final { return true; }

	TypeKind type_kind() const final { return TypeKind::CHAR; }

	String to_string() const final { return "char"; }
};

} // namespace qat::ir

#endif
