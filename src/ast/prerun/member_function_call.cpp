#include "./member_function_call.hpp"

namespace qat::ast {

PreMemberFnCall::PreMemberFnCall(PrerunExpression* _instance, Identifier _memberName, Vec<PrerunExpression*> _arguments,
                                 FileRange _fileRange)
    : PrerunExpression(_fileRange), instance(_instance), memberName(_memberName), arguments(_arguments) {}

PreMemberFnCall* PreMemberFnCall::Create(PrerunExpression* instance, Identifier memberName,
                                         Vec<PrerunExpression*> arguments, FileRange fileRange) {
  return new PreMemberFnCall(instance, memberName, arguments, fileRange);
}

IR::PrerunValue* PreMemberFnCall::emit(IR::Context* ctx) {
  auto inst = instance->emit(ctx);
  if (inst->getType()->isTyped()) {
    auto typed        = inst->getType()->asTyped();
    auto zeroArgCheck = [&]() {
      if (arguments.size() != 0) {
        ctx->Error("The type trait " + ctx->highlightError(memberName.value) +
                       " of wrapped type expects zero arguments, but " +
                       ctx->highlightError(std::to_string(arguments.size())) + " were provided",
                   fileRange);
      }
    };
    if (memberName.value == "byte_size") {
      zeroArgCheck();
      auto* subTy = typed->getSubType();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(
              llvm::Type::getInt64Ty(ctx->llctx),
              subTy->isTypeSized()
                  ? ctx->getMod()->getLLVMModule()->getDataLayout().getTypeStoreSize(subTy->getLLVMType())
                  : 1u),
          IR::UnsignedType::get(64u, ctx));
    } else if (memberName.value == "bit_size") {
      zeroArgCheck();
      auto* subTy = typed->getSubType();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(
              llvm::Type::getInt64Ty(ctx->llctx),
              subTy->isTypeSized()
                  ? ctx->getMod()->getLLVMModule()->getDataLayout().getTypeStoreSizeInBits(subTy->getLLVMType())
                  : 8u),
          IR::UnsignedType::get(64u, ctx));
    } else if (memberName.value == "name") {
      zeroArgCheck();
      auto result = typed->getSubType()->toString();
      return new IR::PrerunValue(
          llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(IR::StringSliceType::get(ctx)->getLLVMType()),
                                    {ctx->builder.CreateGlobalStringPtr(result, ctx->getGlobalStringName(), 0U,
                                                                        ctx->getMod()->getLLVMModule()),
                                     llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), result.length())}),
          IR::StringSliceType::get(ctx));
    } else if (memberName.value == "is_packed") {
      zeroArgCheck();
      if (!typed->getSubType()->getLLVMType()->isStructTy()) {
        ctx->Error("The wrapped type is not a struct and hence the type trait " +
                       ctx->highlightError(memberName.value) + " cannot be used. Make sure to check the " +
                       ctx->highlightError("is_underlying_struct_type") + " trait is satisfied first",
                   fileRange);
      }
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                 typed->getSubType()->getLLVMType()->isStructTy() &&
                                         llvm::cast<llvm::StructType>(typed->getSubType()->getLLVMType())->isPacked()
                                     ? 1u
                                     : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_any_unsigned_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(
              llvm::Type::getInt1Ty(ctx->llctx),
              (typed->getSubType()->isUnsignedInteger() ||
               (typed->getSubType()->isCType() && typed->getSubType()->asCType()->getSubType()->isUnsignedInteger()))
                  ? 1u
                  : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_any_signed_type") {
      zeroArgCheck();
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        (typed->getSubType()->isInteger() ||
                                                         (typed->getSubType()->isCType() &&
                                                          typed->getSubType()->asCType()->getSubType()->isInteger()))
                                                            ? 1u
                                                            : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_any_float_type") {
      zeroArgCheck();
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        (typed->getSubType()->isFloat() ||
                                                         (typed->getSubType()->isCType() &&
                                                          typed->getSubType()->asCType()->getSubType()->isFloat()))
                                                            ? 1u
                                                            : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_unsigned_int_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isUnsignedInteger() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_signed_int_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isInteger() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_float_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isFloat() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_tuple_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isTuple() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_string_slice_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isStringSlice() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_struct_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isCoreType() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_underlying_struct_type") {
      zeroArgCheck();
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        typed->getSubType()->getLLVMType()->isStructTy() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_mix_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isMix() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_array_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isArray() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_choice_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isChoice() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_maybe_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isMaybe() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_result_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isResult() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_void_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isVoid() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_future_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isFuture() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_pointer_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isPointer() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_multi_pointer_type") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(
              llvm::Type::getInt1Ty(ctx->llctx),
              (typed->getSubType()->isPointer() && typed->getSubType()->asPointer()->isMulti()) ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_trivially_copyable") {
      zeroArgCheck();
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        typed->getSubType()->isTriviallyCopyable() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_trivially_movable") {
      zeroArgCheck();
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        typed->getSubType()->isTriviallyMovable() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_copy_constructible") {
      zeroArgCheck();
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        typed->getSubType()->isCopyConstructible() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_copy_assignable") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isCopyAssignable() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_move_constructible") {
      zeroArgCheck();
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        typed->getSubType()->isMoveConstructible() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_move_assignable") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->isMoveAssignable() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "has_copy") {
      zeroArgCheck();
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        (typed->getSubType()->isTriviallyCopyable()) ||
                                                                (typed->getSubType()->isCopyConstructible() &&
                                                                 typed->getSubType()->isCopyAssignable())
                                                            ? 1u
                                                            : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "has_move") {
      zeroArgCheck();
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        (typed->getSubType()->isTriviallyMovable()) ||
                                                                (typed->getSubType()->isMoveConstructible() &&
                                                                 typed->getSubType()->isMoveAssignable())
                                                            ? 1u
                                                            : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "is_default_constructible") {
      zeroArgCheck();
      return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx),
                                                        typed->getSubType()->isDefaultConstructible() ? 1u : 0u),
                                 IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "has_default_value") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->hasDefaultValue() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else if (memberName.value == "can_be_prerun") {
      zeroArgCheck();
      return new IR::PrerunValue(
          llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), typed->getSubType()->canBePrerun() ? 1u : 0u),
          IR::UnsignedType::getBool(ctx));
    } else {
      ctx->Error(ctx->highlightError(memberName.value) + " is not a recognised attribute for the wrapped type",
                 memberName.range);
    }
  } else {
    ctx->Error("Expression is of type " + ctx->highlightError(inst->getType()->toString()) +
                   " and hence doesn't support prerun member function call",
               fileRange);
  }
  return nullptr;
}

Json PreMemberFnCall::toJson() const {
  Vec<JsonValue> argJson;
  for (auto arg : arguments) {
    argJson.push_back(arg->toJson());
  }
  return Json()
      ._("instance", instance->toJson())
      ._("memberName", memberName)
      ._("arguments", argJson)
      ._("fileRange", fileRange);
}

} // namespace qat::ast