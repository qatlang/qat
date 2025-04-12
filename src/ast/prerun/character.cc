#include "./character.hpp"
#include "../../IR/types/char.hpp"
#include "../../IR/types/native_type.hpp"
#include "../../utils/utils.hpp"

#define IS_ONE_BYTE   (bytes[0] & 0x80 == 0)
#define IS_TWO_BYTE   ((bytes[0] & 0xE0) == 0xC0)
#define IS_THREE_BYTE ((bytes[0] & 0xF0) == 0xE0)
#define IS_FOUR_BYTE  ((bytes[0] & 0xF8) == 0xF0)

namespace qat::ast {

ir::PrerunValue* Character::emit(EmitCtx* ctx) {
	if (isByte) {
		auto byteTy = ir::NativeType::get_byte(ctx->irCtx);
		return ir::PrerunValue::get(llvm::ConstantInt::get(byteTy->get_llvm_type(), bytes[0]), byteTy);
	} else {
		auto toHex = [&]() -> String {
			String result;
			result.reserve(12);
			for (u8 i = 0; i < 4; i++) {
				result += " " + utils::to_hex(bytes[i], 2);
			}
			return std::move(result);
		};
		auto charTy = ir::CharType::get(ctx->irCtx->llctx);
		auto valRes = utils::utf8_to_unicode_scalar(bytes);
		if (not valRes.has_value()) {
			ctx->Error("Invalid UTF-8 sequence. The byte sequence found in this " + ctx->color(charTy->to_string()) +
			               " value is " + ctx->color(toHex()),
			           fileRange);
		}
		return ir::PrerunValue::get(llvm::ConstantInt::get(charTy->get_llvm_type(), valRes.value()), charTy);
	}
}

String Character::to_string() const {
	if (isByte) {
		return String("b`") + String(1, bytes[0]) + "`";
	} else {
		String result = "`";
		u8     len    = 1;
		if (IS_TWO_BYTE) {
			len = 2;
		} else if (IS_THREE_BYTE) {
			len = 3;
		} else if (IS_FOUR_BYTE) {
			len = 4;
		}
		for (u8 i = 0; i < len; i++) {
			result.push_back(bytes[i]);
		}
		result += "`";
		return std::move(result);
	}
}

Json Character::to_json() const {
	return Json()
	    ._("nodeType", "character")
	    ._("isByte", isByte)
	    ._("bytes", JsonValue(Vec<JsonValue>{bytes[0], bytes[1], bytes[2], bytes[3]}))
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
