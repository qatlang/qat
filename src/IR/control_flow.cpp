#include "./control_flow.hpp"
#include "../show.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

namespace qat::IR {

// NOLINTBEGIN(readability-identifier-length)
bool hasTerminatorInstruction(llvm::BasicBlock* bb) {
  for (auto& inst : *bb) {
    if (llvm::isa<llvm::BranchInst>(&inst) || llvm::isa<llvm::ReturnInst>(&inst) ||
        llvm::isa<llvm::InvokeInst>(&inst) || llvm::isa<llvm::SwitchInst>(&inst)) {
      return true;
    }
  }
  return false;
}

bool isTerminatorInstruction(llvm::Value* value) {
  return llvm::isa<llvm::BranchInst>(value) || llvm::isa<llvm::ReturnInst>(value) ||
         llvm::isa<llvm::InvokeInst>(value) || llvm::isa<llvm::SwitchInst>(value);
}

llvm::Instruction* addBranch(llvm::IRBuilder<>& builder, llvm::BasicBlock* dest) {
  if (!hasTerminatorInstruction(builder.GetInsertBlock())) {
    return builder.CreateBr(dest);
  }
  return nullptr;
}

// NOLINTEND(readability-identifier-length)

} // namespace qat::IR