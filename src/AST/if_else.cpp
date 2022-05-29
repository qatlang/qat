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

#include "./if_else.hpp"

namespace qat {
namespace AST {

IfElse::IfElse(Expression _condition, Block _if_block,
               std::optional<Block> _else_block,
               std::optional<Block> _merge_block,
               utils::FilePlacement _filePlacement)
    : condition(_condition), if_block(_if_block), else_block(_else_block),
      merge_block(_merge_block.value_or(
          Block(std::vector<Sentence>(),
                utils::FilePlacement(
                    _else_block.value_or(_if_block).file_placement.file,
                    _else_block.value_or(_if_block).file_placement.end,
                    _else_block.value_or(_if_block).file_placement.end)))),
      Sentence(_filePlacement) {}

llvm::Value *IfElse::generate(IR::Generator *generator) {
  auto gen_cond = condition.generate(generator);
  if (gen_cond) {
    if (gen_cond->getType()->isIntegerTy(1)) {
      auto if_bb = if_block.create_bb(generator);
      auto cond_value = generator->builder.CreateICmpEQ(
          gen_cond,
          llvm::ConstantInt::get(generator->llvmContext,
                                 llvm::APInt(1u, 1u, false)),
          if_bb->getName() + "'condition");
      auto else_bb = else_block.has_value()
                         ? else_block.value().create_bb(generator)
                         : nullptr;
      auto merge_bb = merge_block.create_bb(generator);
      generator->builder.CreateCondBr(cond_value, if_bb, else_bb);

      /* If */
      if_block.generate(generator);
      generator->builder.CreateBr(merge_bb);

      /* Else */
      if (else_block.has_value()) {
        else_block.value().generate(generator);
        generator->builder.CreateBr(merge_bb);
      }

      /* Merge - Remaining sentences not part of the conditional branching */
      return merge_block.generate(generator);
    } else {
      generator->throw_error(
          "Condition expression is of the type `" +
              qat::utils::llvmTypeToName(gen_cond->getType()) +
              "`, but `if` sentence expects an expression of `bool`, "
              "`i1` or `u1` type",
          file_placement);
    }
  } else {
    generator->throw_error("Condition expression is null, but `if` sentence "
                           "expects an expression of `bool`, `i1` or `u1` type",
                           file_placement);
  }
}

} // namespace AST
} // namespace qat