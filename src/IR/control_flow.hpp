#ifndef QAT_IR_PASSES_HPP
#define QAT_IR_PASSES_HPP

#include "../utils/macros.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"

namespace qat::IR {

useit bool isTerminatorInstruction(llvm::Value *value);
useit bool hasTerminatorInstruction(llvm::BasicBlock *basicblock);
useit llvm::Instruction *addBranch(llvm::IRBuilder<> &builder,
                                   llvm::BasicBlock  *dest);

// void addConditionalBranch(llvm::IRBuilder<> &builder, llvm::Value *condition,
//                           llvm::BasicBlock *trueBlock,
//                           llvm::BasicBlock *falseBlock);

} // namespace qat::IR

#endif