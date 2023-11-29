#include "./custom_integer_literal.hpp"
#include "../../utils/json.hpp"
#include <string>

namespace qat::ast {

String CustomIntegerLiteral::radixDigits      = "0123456789abcdefghijklmnopqrstuvwxyz";
String CustomIntegerLiteral::radixDigitsUpper = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

CustomIntegerLiteral::CustomIntegerLiteral(String _value, Maybe<bool> _isUnsigned, Maybe<u32> _bitWidth,
                                           Maybe<u8> _radix, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), value(std::move(_value)), bitWidth(_bitWidth), radix(_radix),
      isUnsigned(_isUnsigned) {}

#define DEFAULT_RADIX_VALUE      10u
#define DEFAULT_INTEGER_BITWIDTH 32u

IR::PrerunValue* CustomIntegerLiteral::emit(IR::Context* ctx) {
  bool          numberIsUnsigned = false;
  String        numberValue;
  Maybe<String> suffix;
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
    if (value.find_last_of('_') != String::npos) {
      numberValue = value.substr(0, value.find_last_of('_'));
      suffix      = value.substr(value.find_last_of('_'));
      if (suffix.value() == "_") {
        ctx->Error("Expected a specification for this integer literal. Found just _", fileRange);
      } else {
        suffix = suffix.value().substr(1u);
      }
    } else {
      numberValue = value;
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
    if (!bitWidth) {
      suffix = value.substr(value.find_last_of("_"));
      if (suffix.value() == "_") {
        ctx->Error("Expected a specification for this integer literal. Found just _", fileRange);
      } else {
        suffix = suffix.value().substr(1u);
      }
    }
    auto mainStr = bitWidth ? value : value.substr(0, value.find_last_of("_"));
    for (usize i = 0; i < mainStr.length(); i++) {
      if (mainStr.at(i) != '_') {
        numberValue += mainStr.at(i);
      } else {
        if (i + 1 < mainStr.length()) {
          if (mainStr.at(i + 1) == '_') {
            ctx->Error("Two adjacent underscores found in custom integer literal",
                       FileRange{fileRange.file, FilePos{fileRange.start.line, fileRange.start.character + i},
                                 FilePos{fileRange.start.line, fileRange.start.character + i + 1}});
          }
        }
      }
    }
  }
  if (isUnsigned.has_value() && isUnsigned.value()) {
    numberIsUnsigned = isUnsigned.value();
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
  if (suffix) {
    auto cTypeKind = IR::cTypeKindFromString(suffix.value());
    if (cTypeKind.has_value()) {
      suffixType = IR::CType::getFromCTypeKind(cTypeKind.value(), ctx);
    } else {
      ctx->Error("Invalid suffix for custom integer literal: " + ctx->highlightError(suffix.value()), fileRange);
    }
  }
  return new IR::PrerunValue(
      llvm::ConstantInt::get(suffixType.has_value() ? llvm::cast<llvm::IntegerType>(suffixType.value()->getLLVMType())
                                                    : llvm::Type::getIntNTy(ctx->llctx, usableBitwidth),
                             numberValue, radix.value_or(DEFAULT_RADIX_VALUE)),
      suffixType.has_value() ? suffixType.value()
                             : (numberIsUnsigned ? (IR::QatType*)IR::UnsignedType::get(usableBitwidth, ctx)
                                                 : (IR::QatType*)IR::IntegerType::get(usableBitwidth, ctx)));
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