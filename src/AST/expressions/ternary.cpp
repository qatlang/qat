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

llvm::Value *
qat::AST::TernaryExpression::generate(qat::IR::Generator *generator) {
  llvm::Value *generatedCondition = condition.generate(generator);
  if (generatedCondition != nullptr) {
    if (generatedCondition->getType()->isIntegerTy(1)) {
      llvm::Value *conditionValue = generator->builder.CreateFCmpONE(
          generatedCondition,
          llvm::ConstantFP::get(generator->llvmContext, llvm::APFloat(0.0)),
          "ternaryCondition");
      llvm::Function *parentFunction =
          generator->builder.GetInsertBlock()->getParent();
      llvm::BasicBlock *ifBlock = llvm::BasicBlock::Create(
          generator->llvmContext, "ternaryIfBlock", parentFunction);
      llvm::BasicBlock *elseBlock =
          llvm::BasicBlock::Create(generator->llvmContext, "ternaryElseBlock");
      llvm::BasicBlock *mergeBlock =
          llvm::BasicBlock::Create(generator->llvmContext, "ternaryMergeBlock");
      generator->builder.CreateCondBr(conditionValue, ifBlock, elseBlock);

      generator->builder.SetInsertPoint(ifBlock);
      llvm::Value *ifValue;
      ifValue = ifExpression.generate(generator);
      generator->builder.CreateBr(mergeBlock);
      ifBlock = generator->builder.GetInsertBlock();

      parentFunction->getBasicBlockList().push_back(elseBlock);
      generator->builder.SetInsertPoint(elseBlock);
      llvm::Value *elseValue;
      elseValue = elseExpression.generate(generator);
      generator->builder.CreateBr(mergeBlock);
      elseBlock = generator->builder.GetInsertBlock();

      parentFunction->getBasicBlockList().push_back(mergeBlock);
      generator->builder.SetInsertPoint(mergeBlock);
      llvm::PHINode *phiNode;
      if (ifValue != nullptr && elseValue != nullptr) {
        if (ifValue->getType() != elseValue->getType()) {
          generator->throwError(
              "Ternary expression is giving values of different types",
              filePlacement);
        } else {
          phiNode = generator->builder.CreatePHI(ifValue->getType(), 2,
                                                 "ifElsePhiNode");
          phiNode->addIncoming(ifValue, ifBlock);
          phiNode->addIncoming(elseValue, elseBlock);
          return phiNode;
        }
      } else {
        generator->throwError(
            "Ternary `" + std::string((ifValue == nullptr) ? "if" : "else") +
                "` expression is not giving any value",
            filePlacement);
      }
    } else {
      generator->throwError(
          "Condition expression is of the type `" +
              qat::utilities::llvmTypeToName(generatedCondition->getType()) +
              "`, but ternary expression expects an expression of `bool` or "
              "`i1` type",
          filePlacement);
    }
  } else {
    generator->throwError("Condition expression is null, but `if` sentence "
                          "expects an expression of `bool` or `int<1>` type",
                          filePlacement);
  }
}

qat::AST::NodeType qat::AST::TernaryExpression::nodeType() {
  return NodeType::ternaryExpression;
}
