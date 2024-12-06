#ifndef QAT_IR_PASSES_HPP
#define QAT_IR_PASSES_HPP

#include "../utils/macros.hpp"

#include <llvm/IR/IRBuilder.h>

namespace qat::ir {

useit bool is_terminator_instruction(llvm::Value* value);
useit bool has_terminator_instruction(llvm::BasicBlock* basicblock);
useit llvm::Instruction* add_branch(llvm::IRBuilder<>& builder, llvm::BasicBlock* dest);

// void addConditionalBranch(llvm::IRBuilder<> &builder, llvm::Value *condition,
//                           llvm::BasicBlock *trueBlock,
//                           llvm::BasicBlock *falseBlock);

} // namespace qat::ir

#endif
