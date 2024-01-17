#include "./logic.hpp"
#include "../show.hpp"
#include "./context.hpp"
#include "./function.hpp"
#include "./generics.hpp"
#include "./types/array.hpp"
#include "./types/c_type.hpp"
#include "./types/integer.hpp"
#include "./types/pointer.hpp"
#include "./types/reference.hpp"
#include "./types/unsigned.hpp"
#include "qat_module.hpp"
#include "stdlib.hpp"
#include "types/float.hpp"
#include "types/string_slice.hpp"
#include "types/tuple.hpp"
#include "value.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

IR::Value* Logic::int_to_std_string(bool isSigned, IR::Context* ctx, IR::Value* value, FileRange fileRange) {
  if (IR::StdLib::isStdLibFound() &&
      IR::StdLib::stdLib->hasGenericFunction(isSigned ? "signed_to_string" : "unsigned_to_string",
                                             AccessInfo::GetPrivileged())) {
    auto stringTy      = IR::StdLib::getStringType();
    auto convGenericFn = IR::StdLib::stdLib->getGenericFunction(isSigned ? "signed_to_string" : "unsigned_to_string",
                                                                AccessInfo::GetPrivileged());
    auto intTy  = value->isReference() ? value->getType()->asReference()->getSubType() : value->getType();
    auto convFn = convGenericFn->fillGenerics({IR::GenericToFill::GetType(intTy, fileRange)}, ctx, fileRange);
    if (value->isReference() || value->isImplicitPointer()) {
      value->loadImplicitPointer(ctx->builder);
      if (value->isReference()) {
        value = new IR::Value(ctx->builder.CreateLoad(intTy->getLLVMType(), value->getLLVM()), intTy, false,
                              IR::Nature::temporary);
      }
    }
    auto strRes = convFn->call(ctx, {value->getLLVM()}, None, ctx->getMod());
    ctx->builder.CreateStore(strRes->getLLVM(),
                             ctx->getActiveFunction()
                                 ->getBlock()
                                 ->newValue(ctx->getActiveFunction()->getRandomAllocaName(), stringTy, true, fileRange)
                                 ->getLLVM());
    return strRes;
  } else {
    ctx->Error("Cannot convert integer of type " +
                   ctx->highlightError(value->isReference() ? value->getType()->asReference()->getSubType()->toString()
                                                            : value->getType()->toString()) +
                   " to " + ctx->highlightError("std:String") + " as the standard library function " +
                   ctx->highlightError(isSigned ? "signed_to_string:[T]" : "unsigned_to_string:[T]") +
                   " could not be found",
               fileRange);
  }
  return nullptr;
}

Pair<String, Vec<llvm::Value*>> Logic::formatValues(IR::Context* ctx, Vec<IR::Value*> values, Vec<FileRange> ranges,
                                                    FileRange fileRange) {
  Vec<llvm::Value*> printVals;
  String            formatString;

  std::function<void(IR::Value*, FileRange)> formatValue = [&](IR::Value* val, FileRange valRange) {
    auto valTy = val->getType()->isReference() ? val->getType()->asReference()->getSubType() : val->getType();
    if (valTy->isStringSlice()) {
      formatString += "%.*s";
      if (val->isPrerunValue()) {
        printVals.push_back(val->getLLVMConstant()->getAggregateElement(1u));
        printVals.push_back(val->getLLVMConstant()->getAggregateElement(0u));
      } else if (val->isImplicitPointer() || val->isReference()) {
        auto* strTy = val->isReference() ? val->getType()->asReference()->getSubType() : val->getType();
        if (val->isReference()) {
          val->loadImplicitPointer(ctx->builder);
        }
        printVals.push_back(
            ctx->builder.CreateLoad(llvm::Type::getInt64Ty(ctx->llctx),
                                    ctx->builder.CreateStructGEP(strTy->getLLVMType(), val->getLLVM(), 1u)));
        printVals.push_back(ctx->builder.CreateLoad(
            llvm::Type::getInt8PtrTy(ctx->llctx, ctx->dataLayout.value().getProgramAddressSpace()),
            ctx->builder.CreateStructGEP(strTy->getLLVMType(), val->getLLVM(), 0u)));
      } else {
        printVals.push_back(ctx->builder.CreateExtractValue(val->getLLVM(), {1u}));
        printVals.push_back(ctx->builder.CreateExtractValue(val->getLLVM(), {0u}));
      }
    } else if (valTy->isCType() && valTy->asCType()->isCString()) {
      formatString += "%s";
      if (val->isReference() || val->isImplicitPointer()) {
        val->loadImplicitPointer(ctx->builder);
        if (val->isReference()) {
          val = new IR::Value(ctx->builder.CreateLoad(valTy->getLLVMType(), val->getLLVM()),
                              val->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
        }
      }
      printVals.push_back(val->getLLVM());
    } else if (valTy->isInteger() || (valTy->isCType() && valTy->asCType()->getSubType()->isInteger())) {
      if (val->isPrerunValue()) {
        auto valStr = valTy->toPrerunGenericString(val->asPrerun()).value();
        formatString += valStr;
      } else {
        auto intTy = valTy->isInteger() ? valTy->asInteger() : valTy->asCType()->getSubType()->asInteger();
        // auto strVal =
        //     int_to_std_string(true, ctx, new IR::Value(val->getLLVM(), intTy, false, IR::Nature::temporary),
        //     valRange);
        // formatString += "%.*s";
        // printVals.push_back(ctx->builder.CreateExtractValue(strVal->getLLVM(), {1u}));
        // printVals.push_back(ctx->builder.CreateExtractValue(strVal->getLLVM(), {0u, 0u}));
        llvm::Value* intVal = nullptr;
        if (val->isImplicitPointer() || val->isReference()) {
          if (val->isReference()) {
            val->loadImplicitPointer(ctx->builder);
          }
          intVal = ctx->builder.CreateLoad(valTy->getLLVMType(), val->getLLVM());
        } else {
          intVal = val->getLLVM();
        }
        if (intTy->getBitwidth() < 64u) {
          intVal = ctx->builder.CreateIntCast(intVal, llvm::Type::getInt64Ty(ctx->llctx), true);
        }
        formatString += "%i";
        printVals.push_back(intVal);
      }
    } else if (valTy->isUnsignedInteger() ||
               (valTy->isCType() && valTy->asCType()->getSubType()->isUnsignedInteger())) {
      if (val->isPrerunValue()) {
        auto valStr = valTy->toPrerunGenericString(val->asPrerun()).value();
        formatString += valStr;
      } else {
        auto uintTy = valTy->isUnsignedInteger() ? valTy->asUnsignedInteger()
                                                 : valTy->asCType()->getSubType()->asUnsignedInteger();
        // auto strVal =
        //     int_to_std_string(false, ctx, new IR::Value(val->getLLVM(), uintTy, false, IR::Nature::temporary),
        //     valRange);
        // formatString += "%.*s";
        // printVals.push_back(ctx->builder.CreateExtractValue(strVal->getLLVM(), {1u}));
        // printVals.push_back(ctx->builder.CreateExtractValue(strVal->getLLVM(), {0u, 0u}));
        llvm::Value* intVal = nullptr;
        if (val->isImplicitPointer() || val->isReference()) {
          if (val->isReference()) {
            val->loadImplicitPointer(ctx->builder);
          }
          intVal = ctx->builder.CreateLoad(valTy->getLLVMType(), val->getLLVM());
        } else {
          intVal = val->getLLVM();
        }
        if (uintTy->getBitwidth() < 64u) {
          intVal = ctx->builder.CreateIntCast(intVal, llvm::Type::getInt64Ty(ctx->llctx), false);
        }
        formatString += "%u";
        printVals.push_back(intVal);
      }
    } else if (valTy->isFloat() || (valTy->isCType() && valTy->asCType()->getSubType()->isFloat())) {
      if (val->isPrerunValue()) {
        auto valStr = valTy->toPrerunGenericString(val->asPrerun()).value();
        formatString += valStr;
      } else {
        auto         floatTy  = valTy->isFloat() ? valTy->asFloat() : valTy->asCType()->getSubType()->asFloat();
        llvm::Value* floatVal = nullptr;
        if (val->isImplicitPointer() || val->isReference()) {
          if (val->isReference()) {
            val->loadImplicitPointer(ctx->builder);
          }
          floatVal = ctx->builder.CreateLoad(valTy->getLLVMType(), val->getLLVM());
        } else {
          floatVal = val->getLLVM();
        }
        if (floatTy->getKind() != IR::FloatTypeKind::_64) {
          floatVal = ctx->builder.CreateFPCast(floatVal, llvm::Type::getInt64Ty(ctx->llctx));
        }
        formatString += "%u";
        printVals.push_back(floatVal);
      }
    } else if (valTy->isPointer() || (valTy->isCType() && valTy->asCType()->getSubType()->isPointer())) {
      if (val->isPrerunValue()) {
        auto valStr = valTy->toPrerunGenericString(val->asPrerun()).value();
        formatString += valStr;
      } else {
        if (valTy->asPointer()->isMulti()) {
          auto usizeTy = IR::CType::getUsize(ctx);
          if (val->isReference() || val->isImplicitPointer()) {
            if (val->isReference()) {
              val->loadImplicitPointer(ctx->builder);
            }
            val = new IR::Value(
                ctx->builder.CreateLoad(valTy->asPointer()->getSubType()->getLLVMType()->getPointerTo(
                                            ctx->dataLayout.value().getProgramAddressSpace()),
                                        ctx->builder.CreateStructGEP(valTy->getLLVMType(), val->getLLVM(), 0u)),
                IR::PointerType::get(false, valTy->asPointer()->getSubType(), false, PointerOwner::OfAnonymous(), false,
                                     ctx),
                false, IR::Nature::temporary);
          } else {
            val = new IR::Value(ctx->builder.CreateExtractValue(val->getLLVM(), {0u}),
                                IR::PointerType::get(false, valTy->asPointer()->getSubType(), false,
                                                     PointerOwner::OfAnonymous(), false, ctx),
                                false, IR::Nature::temporary);
          }
        } else {
          if (val->isReference() || val->isImplicitPointer()) {
            val->loadImplicitPointer(ctx->builder);
            if (val->isReference()) {
              val = new IR::Value(ctx->builder.CreateLoad(valTy->getLLVMType(), val->getLLVM()), valTy, false,
                                  IR::Nature::temporary);
            }
          }
        }
        formatString += "%p";
        printVals.push_back(val->getLLVM());
      }
    } else if (valTy->isTuple()) {
      // FIXME - Update when named tuple members are allowed
      formatString += "(";
      auto subTypes = valTy->asTuple()->getSubTypes();
      if (val->isReference() || val->isImplicitPointer()) {
        val->loadImplicitPointer(ctx->builder);
      }
      for (usize i = 0; i < subTypes.size(); i++) {
        auto* subVal = val->getLLVM();
        subVal       = (val->isReference() || val->isImplicitPointer())
                           ? ctx->builder.CreateStructGEP(subTypes.at(i)->getLLVMType(), subVal, i)
                           : (val->isPrerunValue() ? val->getLLVMConstant()->getAggregateElement(i)
                                                   : ctx->builder.CreateExtractValue(subVal, {(unsigned int)i}));
        if (subTypes.at(i)->isReference() && (val->isReference() || val->isImplicitPointer())) {
          subVal = ctx->builder.CreateLoad(subTypes.at(i)->getLLVMType(), subVal);
        }
        formatValue(new IR::Value(subVal,
                                  (val->isReference() || val->isImplicitPointer())
                                      ? IR::ReferenceType::get(val->isReference()
                                                                   ? val->getType()->asReference()->isSubtypeVariable()
                                                                   : val->isVariable(),
                                                               subTypes.at(i), ctx)
                                      : subTypes.at(i),
                                  val->isVariable(), val->getNature()),
                    valRange);
        if (i != (subTypes.size() - 1)) {
          formatString += "; ";
        }
      }
      formatString += ")";
      if (val->isValue()) {
        ctx->builder.CreateStore(val->getLLVM(),
                                 ctx->getActiveFunction()
                                     ->getBlock()
                                     ->newValue(ctx->getActiveFunction()->getRandomAllocaName(), valTy, true, valRange)
                                     ->getLLVM());
      }
    } else if (valTy->isArray()) {
      formatString += "[";
      auto* subType = valTy->asArray()->getElementType();
      if (valTy->asArray()->getLength() > 0) {
        for (usize i = 0; i < valTy->asArray()->getLength(); i++) {
          auto* subVal =
              (val->isReference() || val->isImplicitPointer())
                  ? ctx->builder.CreateInBoundsGEP(valTy->getLLVMType(), val->getLLVM(),
                                                   // TODO - Change index type to usize llvm equivalent
                                                   {llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->llctx), 0u),
                                                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->llctx), i)})
                  : (val->isPrerunValue() ? val->getLLVMConstant()->getAggregateElement(i)
                                          : ctx->builder.CreateExtractValue(val->getLLVM(), {(u32)i}));
          if (subType->isReference() && (val->isReference() || val->isImplicitPointer())) {
            subVal = ctx->builder.CreateLoad(subType->getLLVMType(), subVal);
          }
          formatValue(new IR::Value(subVal,
                                    (val->isReference() || val->isImplicitPointer())
                                        ? (IR::ReferenceType::get(
                                              val->isReference() ? val->getType()->asReference()->isSubtypeVariable()
                                                                 : val->isVariable(),
                                              subType, ctx))
                                        : subType,
                                    val->isVariable(), val->getNature()),
                      valRange);
          if (i != (valTy->asArray()->getLength() - 1)) {
            formatString += ", ";
          }
        }
      }
      formatString += "]";
    } else {
      if ((!val->isPrerunValue()) && valTy->isExpanded() && IR::StdLib::isStdLibFound() &&
          IR::StdLib::hasStringType() && valTy->asExpanded()->hasToConvertor(IR::StdLib::getStringType())) {
        auto stringTy = IR::StdLib::getStringType();
        auto eTy      = valTy->asExpanded();
        auto toFn     = eTy->getToConvertor(stringTy);
        if (!val->isReference() && !val->isImplicitPointer()) {
          auto candVal = ctx->getActiveFunction()->getBlock()->newValue(ctx->getActiveFunction()->getRandomAllocaName(),
                                                                        valTy, true, valRange);
          ctx->builder.CreateStore(val->getLLVM(), candVal->getLLVM());
          val = candVal;
        } else if (val->isReference()) {
          val->loadImplicitPointer(ctx->builder);
        }
        auto stringVal = toFn->call(ctx, {val->getLLVM()}, None, ctx->getMod());
        formatString += "%.*s";
        printVals.push_back(ctx->builder.CreateExtractValue(stringVal->getLLVM(), {1u}));
        printVals.push_back(ctx->builder.CreateExtractValue(stringVal->getLLVM(), {0u, 0u}));
        (void)ctx->getActiveFunction()->getBlock()->newValue(ctx->getActiveFunction()->getRandomAllocaName(), stringTy,
                                                             false, valRange);
      } else if (!val->isPrerunValue() && IR::StdLib::isStdLibFound() && IR::StdLib::hasStringType() &&
                 valTy->isSame(IR::StdLib::getStringType())) {
        auto stringTy = IR::StdLib::getStringType();
        if (val->isReference() || val->isImplicitPointer()) {
          if (val->isReference()) {
            val->loadImplicitPointer(ctx->builder);
          }
          formatString += "%.*s";
          printVals.push_back(
              ctx->builder.CreateLoad(IR::CType::getUsize(ctx)->getLLVMType(),
                                      ctx->builder.CreateStructGEP(stringTy->getLLVMType(), val->getLLVM(), 1u)));
          printVals.push_back(ctx->builder.CreateLoad(
              llvm::Type::getInt8PtrTy(ctx->llctx),
              ctx->builder.CreateStructGEP(stringTy->getLLVMType()->getStructElementType(0u),
                                           ctx->builder.CreateStructGEP(stringTy->getLLVMType(), val->getLLVM(), 0u),
                                           0u)));
        } else {
          formatString += "%.*s";
          printVals.push_back(ctx->builder.CreateExtractValue(val->getLLVM(), {1u}));
          printVals.push_back(ctx->builder.CreateExtractValue(val->getLLVM(), {0u, 0u}));
          ctx->builder.CreateStore(val->getLLVM(), ctx->getActiveFunction()
                                                       ->getBlock()
                                                       ->newValue(ctx->getActiveFunction()->getRandomAllocaName(),
                                                                  IR::StdLib::getStringType(), true, valRange)
                                                       ->getLLVM());
        }
      } else {
        ctx->Error("Cannot format expression of type " + ctx->highlightError(valTy->toString()), valRange);
      }
    }
  };
  for (usize i = 0; i < values.size(); i++) {
    auto val = values.at(i);
    formatValue(val, i < ranges.size() ? ranges[i] : fileRange);
  }
  return {formatString, printVals};
}

void Logic::panicInFunction(IR::Function* fun, Vec<IR::Value*> values, Vec<FileRange> ranges, FileRange fileRange,
                            IR::Context* ctx) {
  fileRange.file     = fs::canonical(fileRange.file);
  auto  startMessage = IR::StringSliceType::create_value(ctx, "\nFunction " + fun->getFullName() + " panicked at " +
                                                                  fileRange.startToString() + " => ");
  auto* mod          = ctx->getMod();
  mod->linkNative(NativeUnit::printf);
  auto              printFn   = mod->getLLVMModule()->getFunction("printf");
  auto              formatRes = formatValues(ctx, values, ranges, fileRange);
  Vec<llvm::Value*> printVals{
      ctx->builder.CreateGlobalStringPtr("%.*s" + formatRes.first + "\n\n", ctx->getGlobalStringName(),
                                         ctx->dataLayout.value().getDefaultGlobalsAddressSpace()),
      startMessage->getLLVMConstant()->getAggregateElement(1u),
      startMessage->getLLVMConstant()->getAggregateElement(0u)};
  for (auto* val : formatRes.second) {
    printVals.push_back(val);
  }
  ctx->builder.CreateCall(printFn->getFunctionType(), printFn, printVals);
  mod->linkNative(NativeUnit::pthreadExit);
  auto pthreadExitFn = mod->getLLVMModule()->getFunction("pthread_exit");
  ctx->builder.CreateCall(
      pthreadExitFn->getFunctionType(), pthreadExitFn,
      {llvm::ConstantPointerNull::get(
          llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo(ctx->dataLayout.value().getProgramAddressSpace()))});
}

String Logic::getGenericVariantName(String mainName, Vec<IR::GenericToFill*>& fills) {
  String result(std::move(mainName));
  result.append(":[");
  SHOW("Initial part of name")
  for (usize i = 0; i < fills.size(); i++) {
    result += fills.at(i)->toString();
    if (i != (fills.size() - 1)) {
      result.append(", ");
    }
  }
  SHOW("Middle part done")
  result.append("]");
  return result;
}

llvm::AllocaInst* Logic::newAlloca(IR::Function* fun, Maybe<String> name, llvm::Type* type) {
  llvm::AllocaInst* result = nullptr;
  llvm::Function*   func   = fun->getLLVMFunction();
  if (func->getEntryBlock().empty()) {
    result = new llvm::AllocaInst(type, 0U, name.value_or(fun->getRandomAllocaName()), &func->getEntryBlock());
  } else {
    llvm::Instruction* inst = nullptr;
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto instr = func->getEntryBlock().begin(); instr != func->getEntryBlock().end(); instr++) {
      if (llvm::isa<llvm::AllocaInst>(&*instr)) {
        continue;
      } else {
        inst = &*instr;
        break;
      }
    }
    if (inst) {
      result = new llvm::AllocaInst(type, 0U, name.value_or(fun->getRandomAllocaName()), inst);
    } else {
      result = new llvm::AllocaInst(type, 0U, name.value_or(fun->getRandomAllocaName()), &func->getEntryBlock());
    }
  }
  return result;
}

bool Logic::compareConstantStrings(llvm::Constant* lhsBuff, llvm::Constant* lhsCount, llvm::Constant* rhsBuff,
                                   llvm::Constant* rhsCount, llvm::LLVMContext& llCtx) {
  if ((*llvm::cast<llvm::ConstantInt>(lhsCount)->getValue().getRawData()) ==
      (*llvm::cast<llvm::ConstantInt>(rhsCount)->getValue().getRawData())) {
    if ((*llvm::cast<llvm::ConstantInt>(lhsCount)->getValue().getRawData()) == 0u) {
      return true;
    }
    return llvm::cast<llvm::ConstantDataArray>(lhsBuff->getOperand(0u))->getAsString() ==
           llvm::cast<llvm::ConstantDataArray>(rhsBuff->getOperand(0u))->getAsString();
  } else {
    return false;
  }
}

} // namespace qat::IR