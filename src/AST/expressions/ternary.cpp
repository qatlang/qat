#include "./ternary.hpp"

namespace qat {
namespace AST {

TernaryExpression::TernaryExpression(Expression *_condition,
                                     Expression *_ifExpression,
                                     Expression *_elseExpression,
                                     utils::FilePlacement _filePlacement)
    : condition(_condition), if_expr(_ifExpression), else_expr(_elseExpression),
      Expression(_filePlacement) {}

llvm::Value *TernaryExpression::emit(qat::IR::Context *ctx) {
  auto gen_cond = condition->emit(ctx);
  if (gen_cond) {
    if (gen_cond->getType()->isIntegerTy(1) ||
        gen_cond->getType()->isIntegerTy(8)) {
      auto parent_fn = ctx->builder.GetInsertBlock()->getParent();
      auto if_block = llvm::BasicBlock::Create(
          ctx->llvmContext, std::to_string(utils::new_block_index(parent_fn)),
          parent_fn);
      auto else_block = llvm::BasicBlock::Create(
          ctx->llvmContext, std::to_string(utils::new_block_index(parent_fn)),
          parent_fn);
      auto mergeBlock = llvm::BasicBlock::Create(
          ctx->llvmContext, std::to_string(utils::new_block_index(parent_fn)),
          parent_fn);
      ctx->builder.CreateCondBr(gen_cond, if_block, else_block);

      /* If block */
      ctx->builder.SetInsertPoint(if_block);
      auto if_val = if_expr->emit(ctx);
      ctx->builder.CreateBr(mergeBlock);

      /* Else block */
      ctx->builder.SetInsertPoint(else_block);
      auto else_val = else_expr->emit(ctx);
      ctx->builder.CreateBr(mergeBlock);

      /* After Block */
      ctx->builder.SetInsertPoint(mergeBlock);
      llvm::PHINode *phiNode;
      if (if_val && else_val) {
        if (if_val->getType() != else_val->getType()) {
          ctx->throw_error(
              "Ternary expression is giving values of different types",
              file_placement);
        } else {
          phiNode =
              ctx->builder.CreatePHI(if_val->getType(), 2, "ifElsePhiNode");
          phiNode->addIncoming(if_val, if_block);
          phiNode->addIncoming(else_val, else_block);
          return phiNode;
        }
      } else {
        ctx->throw_error("Ternary `" +
                             std::string((if_val == nullptr) ? "if" : "else") +
                             "` expression is not giving any value",
                         ((if_val == nullptr) ? if_expr->file_placement
                                              : else_expr->file_placement));
      }
    } else {
      ctx->throw_error(
          "Condition expression is of the type `" +
              qat::utils::llvmTypeToName(gen_cond->getType()) +
              "`, but ternary expression expects an expression of `bool`, "
              "`i1`, `i8`, `u1` or `u8` type",
          if_expr->file_placement);
    }
  } else {
    ctx->throw_error("Condition expression is null, but `if` sentence "
                     "expects an expression of `bool` or `int<1>` type",
                     if_expr->file_placement);
  }
}

void TernaryExpression::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "((";
    condition->emitCPP(file, isHeader);
    file += ") ? (";
    if_expr->emitCPP(file, isHeader);
    file += ") : (";
    else_expr->emitCPP(file, isHeader);
    file += ")) ";
  }
}

backend::JSON TernaryExpression::toJSON() const {
  return backend::JSON()
      ._("nodeType", "ternaryExpression")
      ._("condition", condition->toJSON())
      ._("ifCase", if_expr->toJSON())
      ._("elseCase", else_expr->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat