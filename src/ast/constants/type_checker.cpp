#include "./type_checker.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

#define LLVM_SIZEOF_RESULT_BITWIDTH 64u

TypeChecker::TypeChecker(String _name, Vec<ast::QatType*> _args, FileRange _fileRange)
    : PrerunExpression(std::move(_fileRange)), name(std::move(_name)), args(std::move(_args)) {}

IR::PrerunValue* TypeChecker::emit(IR::Context* ctx) {
  Vec<IR::QatType*> typs;
  for (auto* arg : args) {
    typs.push_back(arg->emit(ctx));
  }
  if (typs.empty()) {
    ctx->Error("At least one type should be provided for type checker", fileRange);
  }
  if ((name == "bits") || (name == "bytes")) {
    if (typs.size() > 1) {
      ctx->Error("Only one type can be provided for " + ctx->highlightError(name) + " type function", fileRange);
    }
    return new IR::PrerunValue(
        llvm::ConstantInt::get(
            llvm::Type::getInt64Ty(ctx->llctx),
            name == "bits"
                ? ctx->getMod()->getLLVMModule()->getDataLayout().getTypeStoreSizeInBits(typs.at(0)->getLLVMType())
                : ctx->getMod()->getLLVMModule()->getDataLayout().getTypeStoreSize(typs.at(0)->getLLVMType())),
        IR::UnsignedType::get(LLVM_SIZEOF_RESULT_BITWIDTH, ctx));
  } else if (name == "isCore") {
    bool res = true;
    for (auto* typ : typs) {
      if (!typ->isCoreType()) {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "isMix") {
    bool res = true;
    for (auto* typ : typs) {
      if (!typ->isMix()) {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "isChoice") {
    bool res = true;
    for (auto* typ : typs) {
      if (!typ->isChoice()) {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "isTuple") {
    bool res = true;
    for (auto* typ : typs) {
      if (!typ->isTuple()) {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "isArray") {
    bool res = true;
    for (auto* typ : typs) {
      if (!typ->isArray()) {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "isTypeDefinition") {
    bool res = true;
    for (auto* typ : typs) {
      if (!typ->isDefinition()) {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "isSigned") {
    bool res = true;
    for (auto* typ : typs) {
      if (!typ->isInteger()) {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "isUnsigned") {
    bool res = true;
    for (auto* typ : typs) {
      if (!typ->isUnsignedInteger()) {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "isIntegral") {
    bool res = true;
    for (auto* typ : typs) {
      if (!typ->isInteger() && !typ->isUnsignedInteger()) {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "isFloatingPoint") {
    bool res = true;
    for (auto* typ : typs) {
      if (!typ->isFloat()) {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "isPointer") {
    bool res = true;
    for (auto* typ : typs) {
      if (!typ->isPointer()) {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "hasDefault") {
    bool res = true;
    for (auto* typ : typs) {
      if (typ->isCoreType()) {
        if (!typ->asCore()->hasDefaultConstructor()) {
          res = false;
          break;
        }
      } else if (typ->isChoice()) {
        if (!typ->asChoice()->hasDefault()) {
          res = false;
          break;
        }
      } else if (typ->isMix()) {
        if (!typ->asMix()->hasDefault()) {
          res = false;
        }
        break;
      } else if (typ->isInteger() || typ->isUnsignedInteger() || typ->isCType() || typ->isStringSlice() ||
                 typ->isFloat()) {
        res = true;
      } else {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "isDestructible") {
    bool res = true;
    for (auto* typ : typs) {
      if (typ->isDestructible()) {
        continue;
      } else {
        res = false;
        break;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "hasAnyConstructor") {
    bool res = true;
    for (auto* typ : typs) {
      if (typ->isExpanded()) {
        if (!typ->asExpanded()->hasAnyConstructor()) {
          res = false;
        }
      } else {
        res = false;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "hasAnyFromConvertor") {
    bool res = true;
    for (auto* typ : typs) {
      if (typ->isExpanded()) {
        if (!typ->asExpanded()->hasAnyFromConvertor()) {
          res = false;
        }
      } else {
        res = false;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "hasCopyConstructor") {
    bool res = true;
    for (auto* typ : typs) {
      if (typ->isExpanded()) {
        if (!typ->asExpanded()->hasCopyConstructor()) {
          res = false;
        }
      } else {
        res = false;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "hasMoveConstructor") {
    bool res = true;
    for (auto* typ : typs) {
      if (typ->isExpanded()) {
        if (!typ->asExpanded()->hasMoveConstructor()) {
          res = false;
        }
      } else {
        res = false;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "hasCopyAssignment") {
    bool res = true;
    for (auto* typ : typs) {
      if (typ->isExpanded()) {
        if (!typ->asExpanded()->hasCopyAssignment()) {
          res = false;
        }
      } else {
        res = false;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "hasMoveAssignment") {
    bool res = true;
    for (auto* typ : typs) {
      if (typ->isExpanded()) {
        if (!typ->asExpanded()->hasMoveAssignment()) {
          res = false;
        }
      } else {
        res = false;
      }
    }
    return new IR::PrerunValue(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, 1u), (res ? 1u : 0u)),
                               IR::UnsignedType::getBool(ctx));
  } else if (name == "name") {
    if (typs.size() > 1) {
      ctx->Error("name type checker expects only one type as argument", fileRange);
    }
    return new IR::PrerunValue(
        llvm::ConstantStruct::get(
            llvm::cast<llvm::StructType>(IR::StringSliceType::get(ctx->llctx)->getLLVMType()),
            // NOTE - This usage of llvm::IRBuilder is allowed as it creates a constant without requiring a function
            {ctx->builder.CreateGlobalStringPtr(typs.at(0)->toString(), ctx->getGlobalStringName(),
                                                ctx->dataLayout->getDefaultGlobalsAddressSpace(),
                                                ctx->getMod()->getLLVMModule()),
             llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), (typs.at(0)->toString().size() + 1))}),
        IR::StringSliceType::get(ctx->llctx));
  } else {
    ctx->Error("Type checker " + ctx->highlightError(name) + " is not supported", fileRange);
  }
  return nullptr;
}

Json TypeChecker::toJson() const {
  Vec<JsonValue> argsJson;
  for (auto* arg : args) {
    argsJson.push_back(arg->toJson());
  }
  return Json()._("nodeType", "typeFunction")._("name", name)._("args", argsJson)._("fileRange", fileRange);
}

} // namespace qat::ast