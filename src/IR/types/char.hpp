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

	useit TypeKind type_kind() const final { return TypeKind::CHAR; }

	useit String to_string() const final { return "char"; }
};

} // namespace qat::ir

#endif
