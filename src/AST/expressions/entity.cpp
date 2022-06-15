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

#include "./entity.hpp"

namespace qat {
namespace AST {

ReferenceEntity Entity::to_reference() {
  return ReferenceEntity(name, file_placement);
}

llvm::Value *Entity::generate(IR::Generator *generator) {
  auto func = generator->builder.GetInsertBlock()->getParent();
  auto &e_block = func->getEntryBlock();
  auto insert_p = e_block.begin();
  llvm::AllocaInst *var_alloca = nullptr;
  llvm::Argument *arg_val = nullptr;
  llvm::LoadInst *var_load = nullptr;
  for (auto &instruction : e_block) {
    if (llvm::isa<llvm::AllocaInst>(instruction)) {
      auto temp_alloc = llvm::dyn_cast<llvm::AllocaInst>(&instruction);
      if (temp_alloc->hasName()) {
        auto var_name = temp_alloc->getName();
        if (var_name.str() == name) {
          var_alloca = temp_alloc;
          break;
        }
      }
    } else {
      break;
    }
  }
  if (var_alloca) {
    var_load = generator->builder.CreateLoad(var_alloca->getType(), var_alloca,
                                             false, llvm::Twine(name));
    utils::Variability::propagate(generator->llvmContext, var_alloca, var_load);
    return var_load;
  } else {
    for (std::size_t i = 0; i < func->arg_size(); i++) {
      auto candidate = func->getArg(i);
      if (candidate->getName().str() == name) {
        arg_val = candidate;
        break;
      }
    }
    if (arg_val) {
      var_load = generator->builder.CreateLoad(arg_val->getType(), arg_val,
                                               false, name);
      utils::Variability::propagate(generator->llvmContext, arg_val, var_load);
      return var_load;
    } else {
      auto globalVariable = generator->get_global_variable(name);
      if (globalVariable) {
        var_load = generator->builder.CreateLoad(globalVariable->getType(),
                                                 globalVariable, false,
                                                 llvm::Twine(name));
        utils::Variability::set(generator->llvmContext, var_load,
                                globalVariable->isConstant());
        return var_load;
      } else {
        for (std::size_t i = 0; i < generator->exposed.size(); i++) {
          auto space_name = generator->exposed.at(i).generate() + name;
          globalVariable = generator->get_global_variable(space_name);
          if (globalVariable) {
            var_load = generator->builder.CreateLoad(
                globalVariable->getValueType(), globalVariable, false,
                llvm::Twine(space_name));
            utils::Variability::set(generator->llvmContext, var_load,
                                    globalVariable->isConstant());
            return var_load;
          }
        }
      }

      generator->throw_error("Entity `" + name +
                                 "` cannot be found in function `" +
                                 e_block.getParent()->getName().str() +
                                 "` and is not a Global Entity",
                             file_placement);
    }
  }
}

} // namespace AST
} // namespace qat