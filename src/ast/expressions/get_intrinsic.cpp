#include "./get_intrinsic.hpp"
#include "../../IR/stdlib.hpp"
#include "../../IR/types/function.hpp"
#include "../../IR/types/vector.hpp"
#include "../../IR/value.hpp"
#include "../types/qat_type.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Intrinsics.h"

namespace qat::ast {

IR::Value* GetIntrinsic::emit(IR::Context* ctx) {
  if (args.empty()) {
    ctx->Error("Expected at least one value to be provided. The first value should be the id of the intrinsic",
               fileRange);
  }
  auto nameIR = args[0]->emit(ctx);
  if (!IR::StdLib::isStdLibFound()) {
    ctx->Error("The standard library could not be found, and hence cannot retrieve intrinsic", fileRange);
  }
  if (!IR::StdLib::stdLib->hasChoiceType("Intrinsic", AccessInfo::GetPrivileged())) {
    ctx->Error("The choice type " + ctx->highlightError("Intrinsic") +
                   " is not found in the standard library, and hence the ID of the intrinsic could not be determined",
               fileRange);
  }
  auto intrChTy = IR::StdLib::stdLib->getChoiceType("Intrinsic", AccessInfo::GetPrivileged());
  if (nameIR->getType()->isSame(intrChTy)) {
    auto nmVal = (u32)(*llvm::cast<llvm::ConstantInt>(nameIR->getLLVMConstant())->getValue().getRawData());
    if (nmVal == (u32)IntrinsicID::llvm_matrix_multiply) {
      if (args.size() == 6) {
        auto firstVal  = args[1]->emit(ctx);
        auto secondVal = args[2]->emit(ctx);
        if (!firstVal->getType()->isTyped()) {
          ctx->Error("Expected a type here, got an expression of type " +
                         ctx->highlightError(firstVal->getType()->toString()),
                     args[1]->fileRange);
        }
        if (!secondVal->getType()->isTyped()) {
          ctx->Error("Expected a type here, got an expression of type " +
                         ctx->highlightError(firstVal->getType()->toString()),
                     args[2]->fileRange);
        }
        if (!(firstVal->getType()->asTyped()->getSubType()->is_vector() &&
              firstVal->getType()->asTyped()->getSubType()->as_vector()->is_fixed())) {
          ctx->Error("The first type should be a fixed vector type, got " +
                         ctx->highlightError(firstVal->getType()->toString()) + " instead",
                     args[1]->fileRange);
        }
        if (!(secondVal->getType()->asTyped()->getSubType()->is_vector() &&
              secondVal->getType()->asTyped()->getSubType()->as_vector()->is_fixed())) {
          ctx->Error("The second type should be a fixed vector type, got " +
                         ctx->highlightError(secondVal->getType()->toString()) + " instead",
                     args[2]->fileRange);
        }
        auto oneTy = firstVal->getType()->asTyped()->getSubType()->as_vector();
        auto twoTy = secondVal->getType()->asTyped()->getSubType()->as_vector();
        if (!oneTy->get_element_type()->isSame(twoTy->get_element_type())) {
          ctx->Error("The first vector type has an element type of " +
                         ctx->highlightError(oneTy->get_element_type()->toString()) +
                         " but the second vector type has an element type of " +
                         ctx->highlightError(twoTy->get_element_type()->toString()),
                     fileRange);
        }
        if (args[3]->hasTypeInferrance()) {
          args[3]->asTypeInferrable()->setInferenceType(IR::UnsignedType::get(32u, ctx));
        }
        if (args[4]->hasTypeInferrance()) {
          args[4]->asTypeInferrable()->setInferenceType(IR::UnsignedType::get(32u, ctx));
        }
        if (args[5]->hasTypeInferrance()) {
          args[5]->asTypeInferrable()->setInferenceType(IR::UnsignedType::get(32u, ctx));
        }
        auto thirdVal  = args[3]->emit(ctx);
        auto fourthVal = args[4]->emit(ctx);
        auto fifthVal  = args[5]->emit(ctx);
        auto checkFn   = [&](IR::PrerunValue* value, FileRange range) {
          if (!(value->getType()->isUnsignedInteger() &&
                (value->getType()->asUnsignedInteger()->getBitwidth() == 32u))) {
            ctx->Error("This value is expected to be of type " + ctx->highlightError("u32") +
                             ". Got an expression of type " + ctx->highlightError(value->getType()->toString()),
                         range);
          }
        };
        checkFn(thirdVal, args[3]->fileRange);
        checkFn(fourthVal, args[4]->fileRange);
        checkFn(fifthVal, args[5]->fileRange);
        auto oneMulRes = llvm::ConstantExpr::getMul(thirdVal->getLLVMConstant(), fourthVal->getLLVMConstant());
        if (!llvm::cast<llvm::ConstantInt>(
                 llvm::ConstantFoldConstant(
                     llvm::ConstantExpr::getICmp(
                         llvm::CmpInst::ICMP_EQ,
                         llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->llctx), oneTy->get_count()), oneMulRes),
                     ctx->dataLayout.value()))
                 ->getValue()
                 .getBoolValue()) {
          ctx->Error(
              "The first type provided is " + ctx->highlightError(oneTy->toString()) +
                  " but the product of the 3rd and 4th values is " +
                  ctx->highlightError(std::to_string(
                      *llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(oneMulRes, ctx->dataLayout.value()))
                           ->getValue()
                           .getRawData())) +
                  ". The element count does not match",
              fileRange);
        }
        auto twoMulRes = llvm::ConstantExpr::getMul(fourthVal->getLLVMConstant(), fifthVal->getLLVMConstant());
        if (!llvm::cast<llvm::ConstantInt>(
                 llvm::ConstantFoldConstant(
                     llvm::ConstantExpr::getICmp(
                         llvm::CmpInst::ICMP_EQ,
                         llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->llctx), twoTy->get_count()), twoMulRes),
                     ctx->dataLayout.value()))
                 ->getValue()
                 .getBoolValue()) {
          ctx->Error(
              "The second type provided is " + ctx->highlightError(oneTy->toString()) +
                  " but the product of the 4th and 5th values is " +
                  ctx->highlightError(std::to_string(
                      *llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(oneMulRes, ctx->dataLayout.value()))
                           ->getValue()
                           .getRawData())) +
                  ". The element count does not match",
              fileRange);
        }
        auto retTy = IR::VectorType::create(
            oneTy->get_element_type(),
            *llvm::cast<llvm::ConstantInt>(
                 llvm::ConstantFoldConstant(
                     llvm::ConstantExpr::getMul(thirdVal->getLLVMConstant(), fifthVal->getLLVMConstant()),
                     ctx->dataLayout.value()))
                 ->getValue()
                 .getRawData(),
            IR::VectorKind::fixed, ctx);
        auto mod = ctx->getMod();
        auto intrFn =
            llvm::Intrinsic::getDeclaration(mod->getLLVMModule(), llvm::Intrinsic::matrix_multiply,
                                            {retTy->getLLVMType(), oneTy->getLLVMType(), twoTy->getLLVMType()});
        auto fnTy = new IR::FunctionType(IR::ReturnType::get(retTy),
                                         {
                                             new IR::ArgumentType(oneTy, false),
                                             new IR::ArgumentType(twoTy, false),
                                             new IR::ArgumentType(thirdVal->getType(), false),
                                             new IR::ArgumentType(thirdVal->getType(), false),
                                             new IR::ArgumentType(thirdVal->getType(), false),
                                         },
                                         ctx->llctx);
        return new IR::Value(intrFn, fnTy, false, IR::Nature::temporary);
      } else {
        ctx->Error(
            "This intrinsic requires 5 parameters to be provided after the Intrinsic ID. The first two parameters are the underlying vectors types of the matrices, the 3rd parameter is the number of rows of the first matrix, the 4th parameter is the number of columns of the first matrix which is equal to the number of rows of the second matrix, and the 5th parameter is the number of columns of the second matrix",
            fileRange);
      }
    } else {
      ctx->Error("This intrinsic is not supported", args[0]->fileRange);
    }
  } else {
    ctx->Error("The first parameter should be the ID of the intrinsic of type " +
                   ctx->highlightError(intrChTy->toString()),
               args[0]->fileRange);
  }
}

Json GetIntrinsic::toJson() const {
  Vec<JsonValue> vals;
  for (auto arg : args) {
    vals.push_back(arg->toJson());
  }
  return Json()._("parameters", vals)._("fileRange", fileRange);
}

} // namespace qat::ast