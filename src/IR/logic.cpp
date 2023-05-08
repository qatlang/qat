#include "./logic.hpp"
#include "../show.hpp"
#include "./function.hpp"
#include "./generics.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

String Logic::getGenericVariantName(String mainName, Vec<IR::GenericToFill*>& fills) {
  String result(std::move(mainName));
  result.append("'<");
  SHOW("Initial part of name")
  for (usize i = 0; i < fills.size(); i++) {
    result += fills.at(i)->toString();
    if (i != (fills.size() - 1)) {
      result.append(", ");
    }
  }
  SHOW("Middle part done")
  result.append(">");
  return result;
}

llvm::AllocaInst* Logic::newAlloca(IR::Function* fun, const String& name, llvm::Type* type) {
  llvm::AllocaInst* result = nullptr;
  llvm::Function*   func   = fun->isAsyncFunction() ? fun->getAsyncSubFunction() : fun->getLLVMFunction();
  if (func->getEntryBlock().empty()) {
    result = new llvm::AllocaInst(type, 0U, name, &func->getEntryBlock());
  } else {
    llvm::Instruction* inst = nullptr;
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto instr = func->getEntryBlock().begin(); instr != func->getEntryBlock().end();
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
    if ((*llvm::cast<llvm::ConstantInt>(lhsCount)->getValue().getRawData()) == 0u) {
      return true;
    }
    return llvm::cast<llvm::ConstantDataArray>(lhsBuff->getOperand(0u))->getAsString() ==
           llvm::cast<llvm::ConstantDataArray>(rhsBuff->getOperand(0u))->getAsString();
  } else {
    SHOW("Constant string comparison result: false")
    return false;
  }
}

} // namespace qat::IR