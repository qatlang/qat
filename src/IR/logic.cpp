#include "./logic.hpp"
#include "../show.hpp"
#include "context.hpp"
#include "function.hpp"
#include "types/core_type.hpp"
#include "types/reference.hpp"
#include "value.hpp"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

String Logic::getTemplateVariantName(String mainName, Vec<IR::QatType*>& types) {
  String result(std::move(mainName));
  result += "'<";
  for (usize i = 0; i < types.size(); i++) {
    result += types.at(i)->toString();
    if (i != (types.size() - 1)) {
      result += ", ";
    }
  }
  result += ">";
  return result;
}

llvm::AllocaInst* Logic::newAlloca(IR::Function* fun, const String& name, llvm::Type* type) {
  llvm::AllocaInst* result = nullptr;
  llvm::Function*   func   = fun->isAsyncFunction() ? fun->getAsyncSubFunction() : fun->getLLVMFunction();
  if (func->getEntryBlock().getInstList().empty()) {
    result = new llvm::AllocaInst(type, 0U, name, &func->getEntryBlock());
  } else {
    llvm::Instruction* inst = nullptr;
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto instr = func->getEntryBlock().getInstList().begin(); instr != func->getEntryBlock().getInstList().end();
         instr++) {
      if (llvm::isa<llvm::AllocaInst>(&*instr)) {
        continue;
      } else {
        inst = &*instr;
        break;
      }
    }
    if (inst) {
      result = new llvm::AllocaInst(type, 0U, name, inst);
    } else {
      result = new llvm::AllocaInst(type, 0U, name, &func->getEntryBlock());
    }
  }
  return result;
}

bool Logic::compareConstantStrings(llvm::Constant* lhsBuff, llvm::Constant* lhsCount, llvm::Constant* rhsBuff,
                                   llvm::Constant* rhsCount, llvm::LLVMContext& llCtx) {
  SHOW("Starting constant string comparison")
  if ((*llvm::cast<llvm::ConstantInt>(lhsCount)->getValue().getRawData()) ==
      (*llvm::cast<llvm::ConstantInt>(rhsCount)->getValue().getRawData())) {
    SHOW("Constant string length matches")
    bool result = true;
    for (usize i = 0; i < (*llvm::cast<llvm::ConstantInt>(lhsCount)->getValue().getRawData()); i++) {
      if (!llvm::cast<llvm::ConstantInt>(
               llvm::ConstantExpr::getICmp(
                   llvm::ICmpInst::ICMP_EQ,
                   llvm::ConstantExpr::getInBoundsGetElementPtr(
                       llvm::Type::getInt8Ty(llCtx), lhsBuff, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llCtx), i)),
                   llvm::ConstantExpr::getInBoundsGetElementPtr(
                       llvm::Type::getInt8Ty(llCtx), rhsBuff,
                       llvm::ConstantInt::get(llvm::Type::getInt64Ty(llCtx), i))))
               ->getValue()
               .getBoolValue()) {
        SHOW("Constant string aren't equal")
        result = false;
        break;
      }
    }
    return result;
  } else {
    return false;
  }
}

} // namespace qat::IR