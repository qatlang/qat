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

#include "./local_declaration.hpp"

qat::AST::LocalDeclaration::LocalDeclaration(
    llvm::Optional<QatType> _type, std::string _name, Expression _value,
    bool _variability, utils::FilePlacement _filePlacement)
    : type(_type), name(_name), value(_value), variability(_variability),
      Sentence(_filePlacement) {}

llvm::Value *
qat::AST::LocalDeclaration::generate(qat::IR::Generator *generator) {
  auto gen_value = value.generate(generator);
  if (type.hasValue()) {
    auto decl_ty = type.getValue().generate(generator);
    if (decl_ty != gen_value->getType()) {
      // FIXME - Remove all implicit casts
      if (decl_ty->isIntegerTy() && gen_value->getType()->isIntegerTy()) {
        gen_value = generator->builder.CreateIntCast(gen_value, decl_ty, true,
                                                     "implicit_cast");
      } else if (decl_ty->isFloatingPointTy() &&
                 gen_value->getType()->isIntegerTy()) {
        generator->builder.CreateSIToFP(gen_value, decl_ty, "implicit_cast");
      } else if (decl_ty->isFloatingPointTy() &&
                 gen_value->getType()->isFloatingPointTy()) {
        gen_value = generator->builder.CreateFPCast(gen_value, decl_ty,
                                                    "implicit_cast");
      } else if (decl_ty->isIntegerTy() &&
                 gen_value->getType()->isFloatingPointTy()) {
        generator->throw_error(
            "Floating Point to Integer casting can be lossy. "
            "Not performing an implicit cast. Please provide "
            "the correct value or add an explicit cast",
            file_placement);
      } else {
        auto gen_ty_name = qat::utils::llvmTypeToName(gen_value->getType());
        auto given_ty_name = qat::utils::llvmTypeToName(decl_ty);
        auto conv_fn =
            generator->get_function(gen_ty_name + "'to'" + given_ty_name);
        if (conv_fn == nullptr) {
          conv_fn =
              generator->get_function(given_ty_name + "'from'" + gen_ty_name);
          if (conv_fn == nullptr) {
            generator->throw_error("Conversion functions `" + gen_ty_name +
                                       "'to'" + given_ty_name + "` and `" +
                                       given_ty_name + "'from'" + gen_ty_name +
                                       "` not found",
                                   file_placement);
          }
        }
        std::vector<llvm::Value *> args;
        args.push_back(gen_value);
        gen_value = generator->builder.CreateCall(
            conv_fn->getFunctionType(), conv_fn,
            llvm::ArrayRef<llvm::Value *>(args), "conversion_call", nullptr);
      }
    }
  }
  auto function = generator->builder.GetInsertBlock()->getParent();
  auto &e_block = function->getEntryBlock();
  auto insert_p = e_block.begin();
  bool entity_exists = false;
  for (std::size_t i = 0; i < function->arg_size(); i++) {
    if (function->getArg(i)->getName().str() == name) {
      entity_exists = true;
      break;
    }
  }
  if (entity_exists) {
    generator->throw_error(
        "`" + name + "` is an argument of the function `" +
            function->getName().str() +
            "`. Please remove this declaration or rename this entity",
        file_placement);
  } else {
    for (auto &instr : e_block) {
      if (llvm::isa<llvm::AllocaInst>(instr)) {
        insert_p++;
        auto candidate = llvm::dyn_cast<llvm::AllocaInst>(&instr);
        if (candidate->hasName() && (candidate->getName().str() == name)) {
          entity_exists = true;
          break;
        }
      } else {
        break;
      }
    }
    if (entity_exists) {
      generator->throw_error("Redeclaration of entity `" + name +
                                 "`. Please remove the previous declaration or "
                                 "rename this entity",
                             file_placement);
    }
  }
  auto old_insert_p = generator->builder.GetInsertPoint();
  generator->builder.SetInsertPoint(&*insert_p);
  auto var_alloca = generator->builder.CreateAlloca(
      (type.hasValue() ? type.getValue().generate(generator)
                       : gen_value->getType()),
      0, nullptr, llvm::Twine(name));
  utils::Variability::set(generator->llvmContext, var_alloca, variability);
  generator->builder.SetInsertPoint(&*old_insert_p);
  auto storeValue =
      generator->builder.CreateStore(gen_value, var_alloca, false);
  return storeValue;
}

qat::AST::NodeType qat::AST::LocalDeclaration::nodeType() {
  return qat::AST::NodeType::localDeclaration;
}