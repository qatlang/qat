#include "./custom_integer_literal.hpp"
#include "../../utils/json.hpp"
#include <string>

namespace qat::ast {

String CustomIntegerLiteral::radixDigits      = "0123456789abcdefghijklmnopqrstuvwxyz";
String CustomIntegerLiteral::radixDigitsUpper = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

#define DEFAULT_RADIX_VALUE      10u
#define DEFAULT_INTEGER_BITWIDTH 32u

IR::PrerunValue* CustomIntegerLiteral::emit(IR::Context* ctx) {
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
                       ctx->highlightError(std::to_string(radix.value())) + ", the characters allowed are " +
                       ctx->highlightError(radixDigits.substr(0, radix.value())),
                   FileRange{fileRange.file,
                             FilePos{fileRange.start.line,
                                     fileRange.start.character + radixToString(radix.value()).length() + i},
                             FilePos{fileRange.start.line,
                                     fileRange.start.character + radixToString(radix.value()).length() + i + 1}});
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
                       FileRange{fileRange.file, FilePos{fileRange.start.line, fileRange.start.character + i},
                                 FilePos{fileRange.start.line, fileRange.start.character + i + 1}});
          }
        }
      }
    }
  }
  if (isUnsigned.has_value() && isUnsigned.value()) {
    numberIsUnsigned = true;
    if (bitWidth && !ctx->getMod()->hasUnsignedBitwidth(bitWidth.value())) {
      ctx->Error("The unsigned integer bitwidth " + ctx->highlightError("u" + std::to_string(bitWidth.value())) +
                     " is not brought into this module. Please bring it using " +
                     ctx->highlightError("bring u" + std::to_string(bitWidth.value()) + "."),
                 fileRange);
    }
  } else {
    if (bitWidth && !ctx->getMod()->hasIntegerBitwidth(bitWidth.value())) {
      ctx->Error("The integer bitwidth " + ctx->highlightError("i" + std::to_string(bitWidth.value())) +
                     " is not brought into this module. Please bring it using " +
                     ctx->highlightError("bring i" + std::to_string(bitWidth.value()) + "."),
                 fileRange);
    }
  }
  u32 usableBitwidth = bitWidth.has_value() ? bitWidth.value() : DEFAULT_INTEGER_BITWIDTH;

  Maybe<IR::QatType*> suffixType;
  if (suffix.has_value()) {
    auto cTypeKind = IR::cTypeKindFromString(suffix.value().value);
    if (cTypeKind.has_value()) {
      suffixType = IR::CType::getFromCTypeKind(cTypeKind.value(), ctx);
    } else {
      ctx->Error("Invalid suffix for custom integer literal: " + ctx->highlightError(suffix.value().value),
                 suffix.value().range);
    }
  }
  if (isTypeInferred()) {
    if (suffixType.has_value() && !suffixType.value()->isSame(inferredType)) {
      ctx->Error("The type inferred from scope for this custom integer literal is " +
                     ctx->highlightError(inferredType->toString()) + " but the type from the provided suffix is " +
                     ctx->highlightError(suffixType.value()->toString()),
                 fileRange);
    } else {
      if (isUnsigned.has_value()) {
        if (isUnsigned.value() && (inferredType->isInteger() ||
                                   (inferredType->isCType() && inferredType->asCType()->getSubType()->isInteger()))) {
          ctx->Error("The inferred type is " + ctx->highlightError(inferredType->toString()) +
                         " which is not an unsigned integer type",
                     fileRange);
        } else if (!isUnsigned.value() &&
                   (inferredType->isUnsignedInteger() ||
                    (inferredType->isCType() && inferredType->asCType()->getSubType()->isUnsignedInteger()))) {
          ctx->Error("The inferred type is " + ctx->highlightError(inferredType->toString()) +
                         " which is not a signed integer type",
                     fileRange);
        }
      }
    }
  }
  return new IR::PrerunValue(
      llvm::ConstantInt::get(suffixType.has_value()
                                 ? llvm::cast<llvm::IntegerType>(suffixType.value()->getLLVMType())
                                 : (isTypeInferred() ? llvm::cast<llvm::IntegerType>(inferredType->getLLVMType())
                                                     : llvm::Type::getIntNTy(ctx->llctx, usableBitwidth)),
                             numberValue, radix.value_or(DEFAULT_RADIX_VALUE)),
      suffixType.has_value()
          ? suffixType.value()
          : (isTypeInferred() ? inferredType
                              : (numberIsUnsigned ? (IR::QatType*)IR::UnsignedType::get(usableBitwidth, ctx)
                                                  : (IR::QatType*)IR::IntegerType::get(usableBitwidth, ctx))));
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

Json CustomIntegerLiteral::toJson() const {
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

String CustomIntegerLiteral::toString() const {
  return (radix ? radixToString(radix.value()) : "") + value +
         (bitWidth ? ("_" + std::to_string(bitWidth.value())) : "");
}

} // namespace qat::ast