#include "./logic.hpp"
#include "../show.hpp"
#include "context.hpp"
#include "function.hpp"
#include "types/core_type.hpp"
#include "types/reference.hpp"
#include "value.hpp"
#include "llvm/IR/ConstantFold.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
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
  SHOW("LHS length: " << (*llvm::cast<llvm::ConstantInt>(lhsCount)->getValue().getRawData())
                      << " RHS length: " << (*llvm::cast<llvm::ConstantInt>(rhsCount)->getValue().getRawData()))
  if ((*llvm::cast<llvm::ConstantInt>(lhsCount)->getValue().getRawData()) ==
      (*llvm::cast<llvm::ConstantInt>(rhsCount)->getValue().getRawData())) {
    SHOW("Constant string length matches")
    return llvm::cast<llvm::ConstantDataArray>(
               llvm::cast<llvm::GlobalVariable>(lhsBuff->getOperand(0u))->getInitializer())
               ->getRawDataValues()
               .str() == llvm::cast<llvm::ConstantDataArray>(
                             llvm::cast<llvm::GlobalVariable>(rhsBuff->getOperand(0u))->getInitializer())
                             ->getRawDataValues()
                             .str();
  } else {
    SHOW("Constant string comparison result: false")
    return false;
  }
}

} // namespace qat::IR