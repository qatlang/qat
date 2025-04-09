#ifndef QAT_IR_TYPES_CHAR_HPP
#define QAT_IR_TYPES_CHAR_HPP

#include "../../utils/qat_region.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class CharType final : public Type {
  public:
	CharType(llvm::LLVMContext& llctx);

	useit static CharType* get(llvm::LLVMContext& llctx) {
		for (auto* typ : allTypes) {
			if (typ->type_kind() == TypeKind::CHAR) {
				return typ->as_char();
			}
		}
		return std::construct_at(OwnNormal(CharType), llctx);
	}

	useit bool can_be_prerun() const final { return true; }

	useit bool can_be_prerun_generic() const final { return true; }

	useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	useit bool is_type_sized() const final { return true; }

	useit TypeKind type_kind() const final { return TypeKind::CHAR; }

	useit String to_string() const final { return "char"; }
};

} // namespace qat::ir

#endif
