#include "./loop_n_times.hpp"
#include "../../utils/unique_id.hpp"
#include "llvm/IR/BasicBlock.h"

namespace qat {
namespace AST {

LoopNTimes::LoopNTimes(Expression *_count, Block *_block, Block *_after,
                       utils::FilePlacement _filePlacement)
    : count(_count), block(_block), after(_after), Sentence(_filePlacement) {}

unsigned LoopNTimes::new_loop_index_id(llvm::BasicBlock *bb) {
  unsigned result = 0;
  bb = &bb->getParent()->getEntryBlock();
  for (auto inst = bb->begin(); inst != bb->end(); inst++) {
    if (llvm::isa<llvm::AllocaInst>(*inst)) {
      auto alloc = llvm::dyn_cast<llvm::AllocaInst>(&*inst);
      if (alloc->getName().str().find("loop:times:index:") !=
          std::string::npos) {
        result++;
      }
    } else {
      break;
    }
  }
  return result;
}

IR::Value *LoopNTimes::emit(IR::Context *ctx) {
  // auto count_val = count->emit(ctx);
  // if (!count_val->getType()->isIntegerTy()) {
  //   ctx->throw_error("Expected an expression of integer type for the "
  //                    "number of times to loop",
  //                    count->file_placement);
  // }
  // auto prev_bb = ctx->builder.GetInsertBlock();
  // auto inst_iter =
  // &ctx->builder.GetInsertBlock()->getParent()->getEntryBlock();
  // llvm::Instruction *last_inst;
  // for (auto inst = inst_iter->begin(); inst != inst_iter->end(); inst++) {
  //   if (!llvm::isa<llvm::AllocaInst>(*inst)) {
  //     last_inst = &*inst;
  //   }
  // }
  // ctx->builder.SetInsertPoint(last_inst);
  // auto loop_bb = block->create_bb(ctx);
  // auto id = std::to_string(new_loop_index_id(ctx->builder.GetInsertBlock()));
  // auto index_alloca = ctx->builder.CreateAlloca(
  //     count_val->getType(), 0, nullptr, "loop:times:index:" + id);
  // index_alloca->setMetadata(
  //     "origin_block",
  //     llvm::MDNode::get(
  //         ctx->llvmContext,
  //         llvm::MDString::get(ctx->llvmContext, loop_bb->getName())));
  // utils::Variability::set(ctx->llvmContext, index_alloca, true);
  // ctx->builder.SetInsertPoint(prev_bb);
  // /**
  //  *  Storing the initial value of the index
  //  *
  //  */
  // ctx->builder.CreateStore(
  //     llvm::ConstantInt::get(
  //         llvm::dyn_cast<llvm::IntegerType>(count_val->getType()), 0U,
  //         false),
  //     index_alloca, false);
  // /**
  //  *  Generate IR for all sentences present within the loop
  //  *
  //  */
  // block->emit(ctx);
  // /**
  //  *  The ending BasicBlock generated might be different
  //  *
  //  */
  // auto loop_end_bb = ctx->builder.GetInsertBlock();
  // index_alloca->setMetadata(
  //     "end_block",
  //     llvm::MDNode::get(
  //         ctx->llvmContext,
  //         llvm::MDString::get(ctx->llvmContext, loop_end_bb->getName())));
  // ctx->builder.CreateStore(
  //     ctx->builder.CreateAdd(
  //         ctx->builder.CreateLoad(index_alloca->getAllocatedType(),
  //                                 index_alloca, false,
  //                                 index_alloca->getName()),
  //         llvm::ConstantInt::get(
  //             llvm::dyn_cast<llvm::IntegerType>(count_val->getType()), 1U,
  //             false) //
  //         ),
  //     index_alloca, false //
  // );
  // auto after_bb = after->create_bb(ctx);
  // ctx->builder.CreateCondBr(
  //     ctx->builder.CreateICmpULT(
  //         ctx->builder.CreateLoad(index_alloca->getAllocatedType(),
  //                                 index_alloca, false,
  //                                 index_alloca->getName()),
  //         count_val, "loop:times:check:" + id),
  //     loop_bb, after_bb);
  // auto after_end = after->emit(ctx);
  // auto after_end_bb = ctx->builder.GetInsertBlock();
  // ctx->builder.SetInsertPoint(prev_bb);
  // // FIXME - The count could be signed or unsigned integer, change logic to
  // // adapt to that
  // ctx->builder.CreateCondBr(
  //     ctx->builder.CreateICmpSGE(
  //         count_val,
  //         llvm::ConstantInt::get(llvm::dyn_cast<llvm::IntegerType>(
  //                                               count_val->getType()),
  //                                           1U, true)),
  //     loop_bb, after_bb);
  // ctx->builder.SetInsertPoint(after_end_bb);
  // return after_end;
}

void LoopNTimes::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    auto loopID = "loop_" + utils::unique_id();
    file.addLoopID(loopID);
    file += ("for (std::size " + loopID + " = 0; " + loopID + " < (");
    count->emitCPP(file, isHeader);
    file += ("); ++" + loopID + ") ");
    block->emitCPP(file, isHeader);
    file.popLastLoopIndex();
    file.setOpenBlock(true);
    after->emitCPP(file, isHeader);
  }
}

} // namespace AST
} // namespace qat