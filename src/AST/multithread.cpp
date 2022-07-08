#include "./multithread.hpp"

#define MULTITHREAD_INDEX "multithread'index"
#define MULTITHREAD_RESULT "multithread:result"

namespace qat {
namespace AST {

Multithread::Multithread(Expression *_count, Block *_main, Block *_after,
                         utils::FilePlacement _filePlacement)
    : count(_count), Sentence(_filePlacement), name(std::nullopt),
      cache_block(new Block({}, count->file_placement)),
      call_block(new Block({}, count->file_placement)),
      join_block(new Block({}, count->file_placement)), block(_main),
      after(_after), type(std::nullopt) {}

Multithread::Multithread(Expression *_count, std::string _name, QatType *_type,
                         Block *_main, Block *_after,
                         utils::FilePlacement _filePlacement)
    : count(_count), name(_name), type(_type),
      cache_block(new Block({}, count->file_placement)),
      call_block(new Block({}, count->file_placement)),
      join_block(new Block({}, count->file_placement)), block(_main),
      after(_after), Sentence(_filePlacement) {}

std::pair<unsigned, unsigned>
Multithread::get_index_and_position(llvm::BasicBlock *entry) {
  unsigned index = 0;
  unsigned position = 0;
  for (auto inst = entry->begin(); inst != entry->end(); ++inst) {
    if (llvm::isa<llvm::AllocaInst>(&(*inst))) {
      if ((llvm::dyn_cast<llvm::AllocaInst>(&(*inst))->getName().str().find(
               "multithread:loop:index") != std::string::npos)) {
        index++;
      }
    } else {
      break;
    }
    position++;
  }
  return {index, position};
}

IR::Value *Multithread::emit(IR::Context *ctx) {
  //   /**
  //    *  Some primitive types
  //    *
  //    */
  //   auto void_ty = llvm::Type::getVoidTy(ctx->llvmContext);
  //   auto voidptr_ty =
  //   llvm::Type::getVoidTy(ctx->llvmContext)->getPointerTo(); auto i32_ty =
  //   llvm::Type::getInt32Ty(ctx->llvmContext); auto i64_ty =
  //   llvm::Type::getInt64Ty(ctx->llvmContext);

  //   auto count_val = count.emit(ctx);
  //   if (count_val->getType()->isIntegerTy()) {
  //     ctx->throw_error(
  //         "Expected an expression of u64 type for the number of threads",
  //         count.file_placement);
  //   }
  //   llvm::PointerType *pthread_ty = nullptr;
  //   if (!(llvm::StructType::getTypeByName(ctx->llvmContext, "pthread_t"))) {
  //     pthread_ty =
  //         llvm::StructType::create(ctx->llvmContext,
  //         "pthread_t")->getPointerTo();
  //   } else {
  //     pthread_ty = llvm::StructType::getTypeByName(ctx->llvmContext,
  //     "pthread_t")
  //                      ->getPointerTo();
  //   }

  //   auto prev_bb = ctx->builder.GetInsertBlock();
  //   auto cache_bb = cache_block.create_bb(ctx);
  //   auto call_bb = cache_block.create_bb(ctx);
  //   auto join_bb = join_block.create_bb(ctx);
  //   auto curr_fn = prev_bb->getParent();
  //   auto thread_fn = ctx->create_function(
  //       "thread:" + utils::unique_id(), {voidptr_ty}, voidptr_ty, false,
  //       llvm::GlobalValue::LinkageTypes::WeakAnyLinkage //
  //   );
  //   thread_fn->getArg(0)->setName(MULTITHREAD_INDEX);
  //   auto block_bb = block.create_bb(ctx, thread_fn);

  //   /* Thread function - Start */
  //   {
  //     ctx->builder.SetInsertPoint(block_bb);
  //     auto parent_fn = block_bb->getParent();
  //     auto index_alloca =
  //         ctx->builder.CreateAlloca(i64_ty, nullptr, MULTITHREAD_INDEX);
  //     ctx->builder.CreateStore(
  //         ctx->builder.CreatePointerCast(parent_fn->getArg(0U),
  //                                        i64_ty->getPointerTo(), ""),
  //         index_alloca, false);
  //     block.emit(ctx);
  //   }
  //   /* Thread Function - End */

  //   auto after_bb = block.create_bb(ctx, curr_fn);
  //   ctx->builder.SetInsertPoint(prev_bb);
  //   ctx->builder.CreateCondBr(
  //       ctx->builder.CreateICmpSGT(
  //           count_val,
  //           llvm::ConstantInt::get(llvm::dyn_cast<llvm::IntegerType>(
  //                                                 count_val->getType()),
  //                                             0, false)),
  //       cache_bb, after_bb);
  //   ctx->builder.SetInsertPoint(cache_bb);
  //   auto pthreads_array_alloca = llvm::CallInst::CreateMalloc(
  //       ctx->builder.GetInsertBlock(),
  //       llvm::Type::getInt32Ty(ctx->llvmContext), pthread_ty,
  //       llvm::ConstantExpr::getSizeOf(pthread_ty), count_val, nullptr,
  //       "multithread:pthread:array" //
  //   );
  //   ctx->builder.CreateBr(call_bb);

  //   auto mt_res_ty =
  //       llvm::StructType::getTypeByName(ctx->llvmContext,
  //       MULTITHREAD_RESULT);
  //   if (!mt_res_ty) {
  //     mt_res_ty = llvm::StructType::create(
  //         ctx->llvmContext,
  //         llvm::ArrayRef<llvm::Type *>({
  //             llvm::Type::getVoidTy(ctx->llvmContext)->getPointerTo(),
  //             llvm::Type::getInt64Ty(ctx->llvmContext),
  //         }),
  //         MULTITHREAD_RESULT, false);
  //   }

  //   // Type of the result of an individual thread
  //   llvm::Type *result_type = nullptr;
  //   // Size of the type of the result of an individual thread
  //   llvm::Value *result_individual_size = nullptr;
  //   // The heap allocated array that is used to store the results of
  //   execution of
  //       // threads
  //       llvm::Instruction *mt_array_malloc = nullptr;
  //   if (name.has_value() && type.has_value()) {
  //     result_type = type.value().emit(ctx);
  //     result_individual_size = llvm::ConstantExpr::getSizeOf(result_type);
  //     mt_array_malloc = llvm::CallInst::CreateMalloc(
  //         ctx->builder.GetInsertBlock(),
  //         llvm::Type::getInt32Ty(ctx->llvmContext), result_type,
  //         result_individual_size, count_val, nullptr,
  //         "multithread:result:array" //
  //     );
  //   }
}

} // namespace AST
} // namespace qat