#include "./control_flow.hpp"
#include "../show.hpp"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

namespace qat::ir {

// NOLINTBEGIN(readability-identifier-length)
bool has_terminator_instruction(llvm::BasicBlock* bb) {
	for (auto& inst : *bb) {
		if (is_terminator_instruction(&inst)) {
			return true;
		}
	}
	return false;
}

bool is_terminator_instruction(llvm::Value* value) {
	return llvm::isa<llvm::BranchInst>(value) || llvm::isa<llvm::ReturnInst>(value) ||
	       llvm::isa<llvm::InvokeInst>(value) || llvm::isa<llvm::SwitchInst>(value) ||
	       llvm::isa<llvm::UnreachableInst>(value) || llvm::isa<llvm::IndirectBrInst>(value) ||
	       llvm::isa<llvm::CallBrInst>(value) || llvm::isa<llvm::ResumeInst>(value) ||
	       llvm::isa<llvm::CatchSwitchInst>(value) || llvm::isa<llvm::CleanupReturnInst>(value) ||
	       llvm::isa<llvm::CatchReturnInst>(value);
}

llvm::Instruction* add_branch(llvm::IRBuilder<>& builder, llvm::BasicBlock* dest) {
	if (not has_terminator_instruction(builder.GetInsertBlock())) {
		return builder.CreateBr(dest);
	}
	return nullptr;
}

// NOLINTEND(readability-identifier-length)

} // namespace qat::ir
