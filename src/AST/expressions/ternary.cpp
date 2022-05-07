/**
 * Qat Programming Language : Copyright 2022 : Aldrin Mathew
 *
 * AAF INSPECTABLE LICENCE - 1.0
 *
 * This project is licensed under the AAF Inspectable Licence 1.0.
 * You are allowed to inspect the source of this project(s) free of
 * cost, and also to verify the authenticity of the product.
 *
 * Unless required by applicable law, this project is provided
 * "AS IS", WITHOUT ANY WARRANTIES OR PROMISES OF ANY KIND, either
 * expressed or implied. The author(s) of this project is not
 * liable for any harms, errors or troubles caused by using the
 * source or the product, unless implied by law. By using this
 * project, or part of it, you are acknowledging the complete terms
 * and conditions of licensing of this project as specified in AAF
 * Inspectable Licence 1.0 available at this URL:
 *
 * https://github.com/aldrinsartfactory/InspectableLicence/
 *
 * This project may contain parts that are not licensed under the
 * same licence. If so, the licences of those parts should be
 * appropriately mentioned in those parts of the project. The
 * Author MAY provide a notice about the parts of the project that
 * are not licensed under the same licence in a publicly visible
 * manner.
 *
 * You are NOT ALLOWED to sell, or distribute THIS project, its
 * contents, the source or the product or the build result of the
 * source under commercial or non-commercial purposes. You are NOT
 * ALLOWED to revamp, rebrand, refactor, modify, the source, product
 * or the contents of this project.
 *
 * You are NOT ALLOWED to use the name, branding and identity of this
 * project to identify or brand any other project. You ARE however
 * allowed to use the name and branding to pinpoint/show the source
 * of the contents/code/logic of any other project. You are not
 * allowed to use the identification of the Authors of this project
 * to associate them to other projects, in a way that is deceiving
 * or misleading or gives out false information.
 */

#include "./ternary.hpp"

qat::AST::TernaryExpression::TernaryExpression(
    Expression _condition, Expression _ifExpression, Expression _elseExpression,
    utilities::FilePlacement _filePlacement)
    : condition(_condition), if_expr(_ifExpression), else_expr(_elseExpression),
      Expression(_filePlacement) {}

llvm::Value *
qat::AST::TernaryExpression::generate(qat::IR::Generator *generator) {
  auto gen_cond = condition.generate(generator);
  if (gen_cond != nullptr) {
    if (gen_cond->getType()->isIntegerTy(1)) {
      auto cond_val = generator->builder.CreateFCmpONE(
          gen_cond,
          llvm::ConstantFP::get(generator->llvmContext, llvm::APFloat(0.0)),
          "ternaryCondition");
      auto parent_fn = generator->builder.GetInsertBlock()->getParent();
      auto if_block = llvm::BasicBlock::Create(generator->llvmContext,
                                               "ternaryIfBlock", parent_fn);
      auto else_block =
          llvm::BasicBlock::Create(generator->llvmContext, "ternaryElseBlock");
      auto mergeBlock =
          llvm::BasicBlock::Create(generator->llvmContext, "ternaryMergeBlock");
      generator->builder.CreateCondBr(cond_val, if_block, else_block);

      generator->builder.SetInsertPoint(if_block);
      auto if_val = if_expr.generate(generator);
      generator->builder.CreateBr(mergeBlock);
      if_block = generator->builder.GetInsertBlock();

      parent_fn->getBasicBlockList().push_back(else_block);
      generator->builder.SetInsertPoint(else_block);
      auto else_value = else_expr.generate(generator);
      generator->builder.CreateBr(mergeBlock);
      else_block = generator->builder.GetInsertBlock();

      parent_fn->getBasicBlockList().push_back(mergeBlock);
      generator->builder.SetInsertPoint(mergeBlock);
      llvm::PHINode *phiNode;
      if (if_val != nullptr && else_value != nullptr) {
        if (if_val->getType() != else_value->getType()) {
          generator->throwError(
              "Ternary expression is giving values of different types",
              file_placement);
        } else {
          phiNode = generator->builder.CreatePHI(if_val->getType(), 2,
                                                 "ifElsePhiNode");
          phiNode->addIncoming(if_val, if_block);
          phiNode->addIncoming(else_value, else_block);
          return phiNode;
        }
      } else {
        generator->throwError(
            "Ternary `" + std::string((if_val == nullptr) ? "if" : "else") +
                "` expression is not giving any value",
            file_placement);
      }
    } else {
      generator->throwError(
          "Condition expression is of the type `" +
              qat::utilities::llvmTypeToName(gen_cond->getType()) +
              "`, but ternary expression expects an expression of `bool` or "
              "`i1` type",
          file_placement);
    }
  } else {
    generator->throwError("Condition expression is null, but `if` sentence "
                          "expects an expression of `bool` or `int<1>` type",
                          file_placement);
  }
}

qat::AST::NodeType qat::AST::TernaryExpression::nodeType() {
  return NodeType::ternaryExpression;
}
