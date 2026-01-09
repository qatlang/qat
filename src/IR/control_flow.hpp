#ifndef QAT_IR_CONTROL_FLOW_HPP
#define QAT_IR_CONTROL_FLOW_HPP

#include "../utils/macros.hpp"

#include <llvm/IR/IRBuilder.h>

namespace qat::ir {

bool is_terminator_instruction(llvm::Value* value);

bool has_terminator_instruction(llvm::BasicBlock* basicblock);

llvm::Instruction* add_branch(llvm::IRBuilder<>& builder, llvm::BasicBlock* dest);

} // namespace qat::ir

#endif
