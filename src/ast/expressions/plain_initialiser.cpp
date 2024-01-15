#include "./plain_initialiser.hpp"
#include "../../IR/logic.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

PlainInitialiser::PlainInitialiser(QatType* _type, Vec<Pair<String, FileRange>> _fields, Vec<Expression*> _fieldValues,
                                   FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type), fields(std::move(_fields)), fieldValues(std::move(_fieldValues)) {
}

IR::Value* PlainInitialiser::emit(IR::Context* ctx) {
  if (type) {
    auto  reqInfo  = ctx->getAccessInfo();
    auto* typeEmit = type->emit(ctx);
    if (typeEmit->isCoreType()) {
      auto* cTy = typeEmit->asCore();
      if (fieldValues.empty()) {
        if (cTy->hasDefaultConstructor()) {
          auto* dFn = cTy->getDefaultConstructor();
          if (dFn->isAccessible(ctx->getAccessInfo())) {
            llvm::Value* alloca = nullptr;
            if (isLocalDecl()) {
              if (localValue->getType()->isMaybe()) {
                alloca = ctx->builder.CreateStructGEP(localValue->getType()->getLLVMType(), localValue->getLLVM(), 1u);
              } else {
                alloca = localValue->getLLVM();
              }
            } else if (irName) {
              localValue = ctx->getActiveFunction()->getBlock()->newValue(irName->value, cTy, isVar, irName->range);
              alloca     = localValue->getLLVM();
            } else {
              alloca = IR::Logic::newAlloca(ctx->getActiveFunction(), None, cTy->getLLVMType());
            }
            (void)dFn->call(ctx, {alloca}, None, ctx->getMod());
            if (isLocalDecl()) {
              return localValue->toNewIRValue();
            } else {
              return new IR::Value(alloca, cTy, true, IR::Nature::pure);
            }
          } else {
            ctx->Error("The default constructor of core type " + ctx->highlightError(cTy->getFullName()) +
                           " is not accessible here",
                       fileRange);
          }
        }
      }
      if (ctx->getActiveFunction()->isMemberFunction()
              ? !((IR::MemberFunction*)ctx->getActiveFunction())->getParentType()->isSame(cTy)
              : true) {
        if (cTy->hasAnyConstructor()) {
          ctx->Error("Core type " + ctx->highlightError(cTy->getFullName()) +
                         " have constructors and hence the plain initialiser "
                         "cannot be used for this type",
                     fileRange);
        } else if (cTy->hasAnyFromConvertor()) {
          ctx->Error("Core type " + ctx->highlightError(cTy->getFullName()) +
                         " has convertor constructors and hence the plain initialiser "
                         "syntax cannot be used for this type",
                     fileRange);
        }
      }
      if (cTy->getMemberCount() != fieldValues.size()) {
        ctx->Error("Core type " + ctx->highlightError(cTy->toString()) + " has " +
                       ctx->highlightError(std::to_string(cTy->getMemberCount())) + " members. But there are only " +
                       ctx->highlightError(std::to_string(fieldValues.size())) + " expressions provided",
                   fileRange);
      }
      if (fields.empty()) {
        for (usize i = 0; i < fieldValues.size(); i++) {
          auto* mem = cTy->getMemberAt(i);
          if (!mem->visibility.isAccessible(reqInfo)) {
            ctx->Error("Member " + ctx->highlightError(mem->name.value) + " of core type " +
                           ctx->highlightError(cTy->getFullName()) +
                           " is not accessible here and hence plain "
                           "initialiser cannot be used for this type",
                       fieldValues.at(i)->fileRange);
          }
          indices.push_back(i);
        }
      } else {
        for (usize i = 0; i < fields.size(); i++) {
          auto fName  = fields.at(i).first;
          auto fRange = fields.at(i).second;
          if (cTy->hasMember(fName)) {
            auto* mem = cTy->getMemberAt(cTy->getIndexOf(fName).value());
            if (!mem->visibility.isAccessible(reqInfo)) {
              ctx->Error("The member " + ctx->highlightError(fName) + " of core type " +
                             ctx->highlightError(cTy->getFullName()) + " is not accessible here",
                         fRange);
            }
            for (usize j = 0; j != i; j++) {
              if (fields.at(i).first == fields.at(j).first) {
                ctx->Error("The value for member " + ctx->highlightError(fields.at(i).first) + " of core type " +
                               ctx->highlightError(cTy->toString()) + " is already provided. Please check logic",
                           fields.at(i).second);
              }
            }
            indices.push_back(cTy->getIndexOf(fields.at(i).first).value());
          } else {
            ctx->Error("The core type " + ctx->highlightError(cTy->toString()) + " does not have a member named " +
                           ctx->highlightError(fields.at(i).first),
                       fields.at(i).second);
          }
        }
      }
      // FIXME - Support default values
      Vec<IR::Value*> irVals;
      for (usize i = 0; i < fieldValues.size(); i++) {
        if (fieldValues.at(i)->hasTypeInferrance()) {
          fieldValues.at(i)->asTypeInferrable()->setInferenceType(cTy->getMemberAt(indices.at(i))->type);
        }
        auto* fVal  = fieldValues.at(i)->emit(ctx);
        auto* memTy = cTy->getMemberAt(i)->type;
        if (fVal->getType()->isSame(memTy) ||
            (memTy->isReference() && memTy->asReference()->getSubType()->isSame(fVal->getType()) &&
             fVal->isImplicitPointer() && (memTy->asReference()->isSubtypeVariable() ? fVal->isVariable() : true)) ||
            (fVal->getType()->isReference() && fVal->getType()->asReference()->getSubType()->isSame(memTy))) {
          irVals.push_back(fVal);
        } else {
          ctx->Error("This expression does not match the type of the "
                     "corresponding member " +
                         ctx->highlightError(cTy->getMemberNameAt(i)) + " of core type " +
                         ctx->highlightError(cTy->toString()) + ". The member is of type " +
                         ctx->highlightError(memTy->toString()) + ", but the expression is of type " +
                         ctx->highlightError(fVal->getType()->toString()),
                     fieldValues.at(i)->fileRange);
        }
      }
      llvm::Value* alloca = nullptr;
      if (isLocalDecl()) {
        if (localValue->getType()->isMaybe()) {
          alloca = ctx->builder.CreateStructGEP(localValue->getType()->getLLVMType(), localValue->getLLVM(), 1u);
        } else {
          alloca = localValue->getLLVM();
        }
      } else if (irName) {
        localValue = ctx->getActiveFunction()->getBlock()->newValue(irName->value, cTy, isVar, irName->range);
        alloca     = localValue->getLLVM();
      } else {
        alloca = ctx->builder.CreateAlloca(cTy->getLLVMType(), ctx->dataLayout->getAllocaAddrSpace());
      }
      for (usize i = 0; i < irVals.size(); i++) {
        auto* memPtr = ctx->builder.CreateStructGEP(cTy->getLLVMType(), alloca, indices.at(i));
        auto  irVal  = irVals.at(i);
        auto  memTy  = cTy->getMemberAt(indices.at(i))->type;
        if (irVal->isImplicitPointer() || irVal->isReference()) {
          if (memTy->isTriviallyCopyable() || memTy->isTriviallyMovable()) {
            if (irVal->isReference()) {
              irVal->loadImplicitPointer(ctx->builder);
            }
            auto* irOrigVal = ctx->builder.CreateLoad(memTy->getLLVMType(), irVal->getLLVM());
            if (!memTy->isTriviallyCopyable()) {
              if (irVal->isReference() && !irVal->getType()->asReference()->isSubtypeVariable()) {
                ctx->Error("This is a reference without variability and hence cannot be trivially moved from",
                           fieldValues.at(i)->fileRange);
              } else if (!irVal->isReference() && !irVal->isVariable()) {
                ctx->Error("This is an expression without variability and hence cannot be trivially moved from",
                           fieldValues.at(i)->fileRange);
              }
              ctx->builder.CreateStore(llvm::Constant::getNullValue(memTy->getLLVMType()), irVal->getLLVM());
              if (irVal->isLocalToFn()) {
                ctx->getActiveFunction()->getBlock()->addMovedValue(irVal->getLocalID().value());
              }
            }
            ctx->builder.CreateStore(irOrigVal, memPtr);
          } else {
            ctx->Error(
                "The member field " + ctx->highlightError(cTy->getMemberNameAt(indices.at(i))) +
                    " expects an expression of type which is not trivially copyable and trivially movable. Please use " +
                    ctx->highlightError("'copy") + " or " + ctx->highlightError("'move") + " accordingly",
                fieldValues.at(i)->fileRange);
          }
        } else {
          ctx->builder.CreateStore(irVal->getLLVM(), memPtr);
        }
      }
      if (isLocalDecl()) {
        return localValue->toNewIRValue();
      } else {
        return new IR::Value(alloca, cTy, true, IR::Nature::pure);
      }
    } else if (typeEmit->isStringSlice()) {
      if (fieldValues.size() == 2) {
        auto* strData  = fieldValues.at(0)->emit(ctx);
        auto* strLen   = fieldValues.at(1)->emit(ctx);
        auto* dataType = strData->getType();
        auto* lenType  = strLen->getType();
        if (dataType->isReference()) {
          dataType = dataType->asReference()->getSubType();
        }
        if (lenType->isReference()) {
          lenType = lenType->asReference()->getSubType();
        }
        if (dataType->isPointer() && dataType->asPointer()->getSubType()->isUnsignedInteger() &&
            dataType->asPointer()->getSubType()->asUnsignedInteger()->isBitWidth(8u)) {
          if (dataType->asPointer()->isMulti()) {
            // FIXME - Change when `check` is added
            // FIXME - Add length confirmation if pointer is multi, compare with
            // the provided length of the string
            if (strData->isReference()) {
              strData->loadImplicitPointer(ctx->builder);
            }
            strData = new IR::Value(
                strData->isValue() ? ctx->builder.CreateExtractValue(strData->getLLVM(), {0u})
                                   : ctx->builder.CreateLoad(
                                         llvm::PointerType::get(llvm::Type::getInt8Ty(ctx->llctx),
                                                                ctx->dataLayout->getProgramAddressSpace()),
                                         ctx->builder.CreateStructGEP(dataType->getLLVMType(), strData->getLLVM(), 0u)),
                IR::PointerType::get(false, IR::UnsignedType::get(8u, ctx), false, IR::PointerOwner::OfAnonymous(),
                                     false, ctx),
                false, IR::Nature::temporary);
          } else {
            if (strData->isImplicitPointer() || strData->getType()->isReference()) {
              if (strData->isReference() && strData->isImplicitPointer()) {
                strData->loadImplicitPointer(ctx->builder);
              }
              strData = new IR::Value(ctx->builder.CreateLoad(dataType->getLLVMType(), strData->getLLVM()), dataType,
                                      false, IR::Nature::temporary);
            }
          }
          if (lenType->isUnsignedInteger() && lenType->asUnsignedInteger()->getBitwidth() == 64u) {
            if (strLen->isImplicitPointer() || strLen->isReference()) {
              strLen = new IR::Value(ctx->builder.CreateLoad(lenType->getLLVMType(), strLen->getLLVM()), lenType, false,
                                     IR::Nature::temporary);
            }
            auto* strSliceTy = IR::StringSliceType::get(ctx);
            auto* strAlloca  = IR::Logic::newAlloca(ctx->getActiveFunction(), None, strSliceTy->getLLVMType());
            ctx->builder.CreateStore(strData->getLLVM(),
                                     ctx->builder.CreateStructGEP(strSliceTy->getLLVMType(), strAlloca, 0));
            ctx->builder.CreateStore(strLen->getLLVM(),
                                     ctx->builder.CreateStructGEP(strSliceTy->getLLVMType(), strAlloca, 1));
            return new IR::Value(strAlloca, strSliceTy, false, IR::Nature::pure);
          } else {
            ctx->Error("The second argument for creating a string slice is not "
                       "a 64-bit unsigned integer",
                       fieldValues.at(1)->fileRange);
          }
        } else {
          ctx->Error("You are creating an " + ctx->highlightError("str") +
                         " value from 2 arguments and the first argument "
                         "should be of " +
                         ctx->highlightError("ptr:[u8?]") + " or " + ctx->highlightError("multiptr:[u8 ?]") + " type",
                     fieldValues.at(0)->fileRange);
        }
      } else if (fieldValues.size() == 1) {
        auto* strData   = fieldValues.at(0)->emit(ctx);
        auto* strDataTy = strData->getType();
        if (strDataTy->isReference()) {
          strDataTy = strDataTy->asReference()->getSubType();
        }
        if (strDataTy->isPointer() && strDataTy->asPointer()->isMulti() &&
            (strDataTy->asPointer()->getSubType()->isUnsignedInteger() &&
             strDataTy->asPointer()->getSubType()->asUnsignedInteger()->isBitWidth(8u))) {
          auto* strTy = IR::StringSliceType::get(ctx);
          if (strData->isImplicitPointer() || strData->isReference()) {
            if (strData->isReference()) {
              strData->loadImplicitPointer(ctx->builder);
            }
            return new IR::Value(
                ctx->builder.CreatePointerCast(
                    strData->getLLVM(),
                    llvm::PointerType::get(strTy->getLLVMType(), ctx->dataLayout->getProgramAddressSpace())),
                IR::ReferenceType::get(strData->isImplicitPointer()
                                           ? strData->isVariable()
                                           : strData->getType()->asReference()->isSubtypeVariable(),
                                       strTy, ctx),
                false, IR::Nature::temporary);
          } else {
            return new IR::Value(ctx->builder.CreateBitCast(strData->getLLVM(), strDataTy->getLLVMType()), strDataTy,
                                 false, IR::Nature::temporary);
          }
        } else {
          ctx->Error("While creating a " + ctx->highlightError("str") +
                         " value with one argument, the argument is expected "
                         "to be of type " +
                         ctx->highlightError("multiptr:[u8 ?]"),
                     fieldValues.at(0)->fileRange);
        }
      } else {
        ctx->Error("There are two ways to create a " + ctx->highlightError("str") +
                       " value using a plain initialiser. The first way "
                       "requires one argument of type " +
                       ctx->highlightError("multiptr:[u8 ?]") +
                       ". The second way requires 2 arguments. In two possible "
                       "ways:\n1) " +
                       ctx->highlightError("ptr:[u8 ?]") + " and " + ctx->highlightError("u64") + "\n2) " +
                       ctx->highlightError("multiptr:[u8 ?]") + " and " + ctx->highlightError("u64") +
                       "\nIn case you are providing two arguments, the first "
                       "argument is supposed to be a pointer" +
                       " to a start of the data and the second argument is "
                       "supposed to be the number of characters," +
                       " EXCLUDING the null character (if there is any)",
                   fileRange);
      }
    } else {
      ctx->Error("The type is " + ctx->highlightError(typeEmit->toString()) +
                     " and hence cannot be used in a plain "
                     "initialiser",
                 type->fileRange);
    }
  } else {
    ctx->Error("No type provided for plain initialisation", fileRange);
  }
  return nullptr;
}

Json PlainInitialiser::toJson() const {
  return Json()._("nodeType", "plainInitialiser")._("type", type->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast
