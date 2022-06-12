#include "./ternary.hpp"

namespace qat {
namespace AST {

TernaryExpression::TernaryExpression(Expression _condition,
                                     Expression _ifExpression,
                                     Expression _elseExpression,
                                     utils::FilePlacement _filePlacement)
    : condition(_condition), if_expr(_ifExpression), else_expr(_elseExpression),
      Expression(_filePlacement) {}

llvm::Value *TernaryExpression::generate(qat::IR::Generator *generator) {
  auto gen_cond = condition.generate(generator);
  if (gen_cond) {
    if (gen_cond->getType()->isIntegerTy(1) ||
        gen_cond->getType()->isIntegerTy(8)) {
      auto parent_fn = generator->builder.GetInsertBlock()->getParent();
      auto if_block = llvm::BasicBlock::Create(
          generator->llvmContext,
          std::to_string(utils::new_block_index(parent_fn)), parent_fn);
      auto else_block = llvm::BasicBlock::Create(
          generator->llvmContext,
          std::to_string(utils::new_block_index(parent_fn)), parent_fn);
      auto mergeBlock = llvm::BasicBlock::Create(
          generator->llvmContext,
          std::to_string(utils::new_block_index(parent_fn)), parent_fn);
      generator->builder.CreateCondBr(gen_cond, if_block, else_block);

      /* If block */
      generator->builder.SetInsertPoint(if_block);
      auto if_val = if_expr.generate(generator);
      generator->builder.CreateBr(mergeBlock);

      /* Else block */
      generator->builder.SetInsertPoint(else_block);
      auto else_val = else_expr.generate(generator);
      generator->builder.CreateBr(mergeBlock);

      /* After Block */
      generator->builder.SetInsertPoint(mergeBlock);
      llvm::PHINode *phiNode;
      if (if_val && else_val) {
        if (if_val->getType() != else_val->getType()) {
          generator->throw_error(
              "Ternary expression is giving values of different types",
              file_placement);
        } else {
          phiNode = generator->builder.CreatePHI(if_val->getType(), 2,
                                                 "ifElsePhiNode");
          phiNode->addIncoming(if_val, if_block);
          phiNode->addIncoming(else_val, else_block);
          return phiNode;
        }
      } else {
        generator->throw_error(
            "Ternary `" + std::string((if_val == nullptr) ? "if" : "else") +
                "` expression is not giving any value",
            ((if_val == nullptr) ? if_expr.file_placement
                                 : else_expr.file_placement));
      }
    } else {
      generator->throw_error(
          "Condition expression is of the type `" +
              qat::utils::llvmTypeToName(gen_cond->getType()) +
              "`, but ternary expression expects an expression of `bool`, "
              "`i1`, `i8`, `u1` or `u8` type",
          if_expr.file_placement);
    }
  } else {
    generator->throw_error("Condition expression is null, but `if` sentence "
                           "expects an expression of `bool` or `int<1>` type",
                           if_expr.file_placement);
  }
}

} // namespace AST
} // namespace qat