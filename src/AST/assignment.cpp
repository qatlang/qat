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

#include "./assignment.hpp"

namespace qat {
namespace AST {

Assignment::Assignment(std::string _name, Expression _value,
                       utils::FilePlacement _filePlacement)
    : name(_name), value(_value), Sentence(_filePlacement) {}

llvm::Value *Assignment::generate(qat::IR::Generator *generator) {
  auto gen_val = value.generate(generator);
  auto &e_block =
      generator->builder.GetInsertBlock()->getParent()->getEntryBlock();
  auto insert_p = e_block.begin();
  llvm::AllocaInst *var_alloca = nullptr;
  for (auto &instr : e_block) {
    if (llvm::isa<llvm::AllocaInst>(instr)) {
      auto tmp_alloc = llvm::dyn_cast<llvm::AllocaInst>(&instr);
      if (tmp_alloc->hasName()) {
        auto var_name = tmp_alloc->getName();
        if (var_name.str() == name) {
          var_alloca = tmp_alloc;
          break;
        }
      }
    } else {
      break;
    }
  }
  llvm::StoreInst *var_store = nullptr;
  if (var_alloca) {
    if (utils::Variability::get(var_alloca)) {
      var_store = generator->builder.CreateStore(gen_val, var_alloca, false);
    } else {
      generator->throw_error(
          name + " is not a variable value and cannot be reassigned",
          file_placement);
    }
  } else {
    auto global_var = generator->get_global_variable(name);
    if (global_var) {
      if (!global_var->isConstant()) {
        var_store = generator->builder.CreateStore(gen_val, global_var, false);
      } else {
        generator->throw_error(
            "Global entity " + name +
                " is not a variable and cannot be reassigned.",
            file_placement);
      }
    }
  }
  if (var_store) {
    return var_store;
  } else {
    generator->throw_error("The name `" + name + "` not found in function " +
                               e_block.getParent()->getName().str() +
                               " and is not a Global Variable",
                           file_placement);
  }
}

} // namespace AST
} // namespace qat
