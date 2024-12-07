#include "./custom_integer_literal.hpp"
#include "../../utils/json.hpp"
#include <string>

namespace qat::ast {

String CustomIntegerLiteral::radixDigits	  = "0123456789abcdefghijklmnopqrstuvwxyz";
String CustomIntegerLiteral::radixDigitsUpper = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

#define DEFAULT_RADIX_VALUE		 10u
#define DEFAULT_INTEGER_BITWIDTH 32u

ir::PrerunValue* CustomIntegerLiteral::emit(EmitCtx* ctx) {
	bool   numberIsUnsigned = false;
	String numberValue;
	SHOW("ast::CustomIntegerLiteral: " << value << " Has radix: " << (radix.has_value() ? "true" : "false"))
	if (radix) {
		SHOW("Has radix")
		u32 underscoreCount = 0;
		for (auto dig : value) {
			if (dig == '_') {
				underscoreCount++;
			}
		}
		if (underscoreCount > 1) {
			ctx->Error("Separating digits using _ is not allowed in custom radix integer literals", fileRange);
		}
		for (auto& numChar : value) {
			if (numChar != '_') {
				numberValue += numChar;
			}
		}
		for (usize i = 0; i < numberValue.length(); i++) {
			if ((radixDigits.substr(0, radix.value()).find(numberValue.at(i)) == String::npos) &&
				(radixDigitsUpper.substr(0, radix.value()).find(numberValue.at(i)) == String::npos)) {
				ctx->Error("Invalid character found in radix integer literal. For radix " +
							   ctx->color(std::to_string(radix.value())) + ", the characters allowed are " +
							   ctx->color(radixDigits.substr(0, radix.value())),
						   FileRange{fileRange.file,
									 FilePos{fileRange.start.line,
											 fileRange.start.character + radixToString(radix.value()).length() + i},
									 FilePos{fileRange.start.line, fileRange.start.character +
																	   radixToString(radix.value()).length() + i + 1}});
			}
		}
	} else {
		for (usize i = 0; i < value.length(); i++) {
			if (value.at(i) != '_') {
				numberValue += value.at(i);
			} else {
				if (i + 1 < value.length()) {
					if (value.at(i + 1) == '_') {
						ctx->Error("Two adjacent underscores found in custom integer literal",
								   FileRange{fileRange.file,
											 FilePos{fileRange.start.line, fileRange.start.character + i},
											 FilePos{fileRange.start.line, fileRange.start.character + i + 1}});
					}
				}
			}
		}
	}
	if (isUnsigned.has_value() && isUnsigned.value()) {
		numberIsUnsigned = true;
		if (bitWidth && !ctx->mod->has_unsigned_bitwidth(bitWidth.value())) {
			ctx->Error("The unsigned integer bitwidth " + ctx->color("u" + std::to_string(bitWidth.value())) +
						   " is not brought into this module. Please bring it using " +
						   ctx->color("bring u" + std::to_string(bitWidth.value()) + "."),
					   fileRange);
		}
	} else {
		if (bitWidth && !ctx->mod->has_integer_bitwidth(bitWidth.value())) {
			ctx->Error("The integer bitwidth " + ctx->color("i" + std::to_string(bitWidth.value())) +
						   " is not brought into this module. Please bring it using " +
						   ctx->color("bring i" + std::to_string(bitWidth.value()) + "."),
					   fileRange);
		}
	}
	u32 usableBitwidth = bitWidth.has_value() ? bitWidth.value() : DEFAULT_INTEGER_BITWIDTH;

	Maybe<ir::Type*> suffixType;
	if (suffix.has_value()) {
		auto cTypeKind = ir::native_type_kind_from_string(suffix.value().value);
		if (cTypeKind.has_value()) {
			suffixType = ir::NativeType::get_from_kind(cTypeKind.value(), ctx->irCtx);
		} else {
			ctx->Error("Invalid suffix for custom integer literal: " + ctx->color(suffix.value().value),
					   suffix.value().range);
		}
	}
	if (is_type_inferred()) {
		if (suffixType.has_value() && !suffixType.value()->is_same(inferredType)) {
			ctx->Error("The type inferred from scope for this custom integer literal is " +
						   ctx->color(inferredType->to_string()) + " but the type from the provided suffix is " +
						   ctx->color(suffixType.value()->to_string()),
					   fileRange);
		} else {
			if (isUnsigned.has_value()) {
				if (isUnsigned.value() &&
					(inferredType->is_integer() ||
					 (inferredType->is_ctype() && inferredType->as_ctype()->get_subtype()->is_integer()))) {
					ctx->Error("The inferred type is " + ctx->color(inferredType->to_string()) +
								   " which is not an unsigned integer type",
							   fileRange);
				} else if (!isUnsigned.value() && (inferredType->is_unsigned_integer() ||
												   (inferredType->is_ctype() &&
													inferredType->as_ctype()->get_subtype()->is_unsigned_integer()))) {
					ctx->Error("The inferred type is " + ctx->color(inferredType->to_string()) +
								   " which is not a signed integer type",
							   fileRange);
				}
			}
		}
	}
	return ir::PrerunValue::get(
		llvm::ConstantInt::get(suffixType.has_value()
								   ? llvm::cast<llvm::IntegerType>(suffixType.value()->get_llvm_type())
								   : (is_type_inferred() ? llvm::cast<llvm::IntegerType>(inferredType->get_llvm_type())
														 : llvm::Type::getIntNTy(ctx->irCtx->llctx, usableBitwidth)),
							   numberValue, radix.value_or(DEFAULT_RADIX_VALUE)),
		suffixType.has_value()
			? suffixType.value()
			: (is_type_inferred() ? inferredType
								  : (numberIsUnsigned ? (ir::Type*)ir::UnsignedType::create(usableBitwidth, ctx->irCtx)
													  : (ir::Type*)ir::IntegerType::get(usableBitwidth, ctx->irCtx))));
}

String CustomIntegerLiteral::radixToString(u8 val) {
	switch (val) {
		case 2u:
			return "0b";
		case 8u:
			return "0c";
		case 16u:
			return "0x";
		default:
			return "0r" + std::to_string(val) + "_";
	}
}

Json CustomIntegerLiteral::to_json() const {
	return Json()
		._("nodeType", "customIntegerLiteral")
		._("isUnsignedHasValue", isUnsigned.has_value())
		._("isUnsigned", isUnsigned.has_value() ? isUnsigned.value() : JsonValue())
		._("hasRadix", radix.has_value())
		._("radix", radix.has_value() ? radix.value() : JsonValue())
		._("hasBitwidth", bitWidth.has_value())
		._("bitWidth", bitWidth ? bitWidth.value() : JsonValue())
		._("value", value)
		._("fileRange", fileRange);
}

String CustomIntegerLiteral::to_string() const {
	return (radix ? radixToString(radix.value()) : "") + value +
		   (bitWidth ? ("_" + std::to_string(bitWidth.value())) : "");
}

} // namespace qat::ast
