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

#include "./function_call.hpp"

llvm::Value *qat::AST::FunctionCall::generate(qat::IR::Generator *generator) {
  llvm::Function *fn;
  llvm::Function *caller = generator->builder.GetInsertBlock()->getParent();
  if (generator->does_function_exist(name)) {
    /**
     * @brief This is a normal check. The plain name provided is checked.
     *
     */
    fn = generator->get_function(name);
  } else {
    if (caller->getName().find(':', 0) != std::string::npos) {
      auto box = caller->getName()
                     .substr(0, caller->getName().find_last_of(':'))
                     .str();
      auto possibilities = qat::utils::boxPossibilities(box);
      /**
       * Looping in reverse order so that the deeply nested boxes will have
       * higher priority
       */
      for (std::size_t i = (possibilities.size() - 1); i >= 0; i--) {
        if (generator->does_function_exist(possibilities.at(i) + name)) {
          fn = generator->get_function(possibilities.at(i) + name);
          break;
        }
      }
    } else {
      /**
       * Looping in reverse order so that the latest exposed boxes will have
       * higher priority
       */
      for (std::size_t i = generator->exposed_boxes.size() - 1; i >= 0; i--) {
        std::string gen_name = generator->exposed_boxes.at(i).generate() + name;
        if (generator->does_function_exist(gen_name)) {
          fn = generator->get_function(name);
          break;
        }
      }
    }
  }
  if (fn) {
    std::vector<llvm::Value *> generatedValues;
    for (std::size_t i = 0; i < arguments.size(); i++) {
      generatedValues.push_back(arguments.at(i).generate(generator));
    }
    if (fn->arg_size() != arguments.size()) {
      generator->throw_error("Number of arguments passed to the function `" +
                                 fn->getName().str() + "` do not match!",
                             file_placement);
    }
    return generator->builder.CreateCall(
        fn->getFunctionType(), fn,
        llvm::ArrayRef<llvm::Value *>(generatedValues),
        llvm::Twine("Call to " + fn->getName()), nullptr);
  } else {
    generator->throw_error("Function " + name + " does not exist",
                           file_placement);
  }
}

qat::AST::NodeType qat::AST::FunctionCall::nodeType() {
  return qat::AST::NodeType::functionCall;
}
