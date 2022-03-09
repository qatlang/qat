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

llvm::Value *
qat::AST::LocalDeclaration::generate(qat::IR::Generator *generator) {
  llvm::Value *genValue = value.generate(generator);
  if (type.hasValue()) {
    llvm::Type *declType = type.getValue().generate(generator);
    if (declType != genValue->getType()) {
      if (declType->isIntegerTy() && genValue->getType()->isIntegerTy()) {
        genValue = generator->builder.CreateIntCast(genValue, declType, true,
                                                    "implicit_cast");
      } else if (declType->isFloatingPointTy() &&
                 genValue->getType()->isIntegerTy()) {
        generator->builder.CreateSIToFP(genValue, declType, "implicit_cast");
      } else if (declType->isFloatingPointTy() &&
                 genValue->getType()->isFloatingPointTy()) {
        genValue = generator->builder.CreateFPCast(genValue, declType,
                                                   "implicit_cast");
      } else if (declType->isIntegerTy() &&
                 genValue->getType()->isFloatingPointTy()) {
        generator->throwError("Floating Point to Integer casting can be lossy. "
                              "Not performing an implicit cast. Please provide "
                              "the correct value or add an explicit cast",
                              filePosition);
      } else {
        std::string genType =
            qat::utilities::llvmTypeToName(genValue->getType());
        std::string givenType = qat::utilities::llvmTypeToName(declType);
        llvm::Function *conversionFunction =
            generator->getFunction(genType + "'to'" + givenType);
        if (conversionFunction == nullptr) {
          conversionFunction =
              generator->getFunction(givenType + "'from'" + genType);
          if (conversionFunction == nullptr) {
            generator->throwError("Conversion functions `" + genType + "'to'" +
                                      givenType + "` or `" + givenType +
                                      "'from'" + genType + "` not found",
                                  filePosition);
          }
        }
        std::vector<llvm::Value *> args;
        args.push_back(genValue);
        genValue = generator->builder.CreateCall(
            conversionFunction->getFunctionType(), conversionFunction,
            llvm::ArrayRef<llvm::Value *>(args), "conversion_call", nullptr);
      }
    }
  }
  llvm::Function *function = generator->builder.GetInsertBlock()->getParent();
  llvm::BasicBlock &entryBlock = function->getEntryBlock();
  llvm::BasicBlock::iterator insertPoint = entryBlock.begin();
  bool variableExists = false;
  for (std::size_t i = 0; i < function->arg_size(); i++) {
    if (function->getArg(i)->getName().str() == name) {
      variableExists = true;
      break;
    }
  }
  if (variableExists) {
    generator->throwError(
        "`" + name + "` is an argument of the function `" +
            function->getName().str() +
            "`. Please remove this declaration or rename this variable",
        filePosition);
  } else {
    for (auto &instruction : entryBlock) {
      if (llvm::isa<llvm::AllocaInst>(instruction)) {
        insertPoint++;
        llvm::AllocaInst *existingVariable =
            llvm::dyn_cast<llvm::AllocaInst>(&instruction);
        if (existingVariable->hasName() &&
            (existingVariable->getName().str() == name)) {
          variableExists = true;
          break;
        }
      } else {
        break;
      }
    }
    if (variableExists) {
      generator->throwError("Redeclaration of variable `" + name +
                                "`. Please remove the previous declaration or "
                                "rename this variable",
                            filePosition);
    }
  }
  llvm::BasicBlock::iterator oldInsertPoint =
      generator->builder.GetInsertPoint();
  generator->builder.SetInsertPoint(&*insertPoint);
  llvm::AllocaInst *variableAlloca = generator->builder.CreateAlloca(
      (type.hasValue() ? type.getValue().generate(generator)
                       : genValue->getType()),
      0, nullptr, llvm::Twine(name));
  llvm::MDNode *varNatureMD = llvm::MDNode::get(
      generator->llvmContext,
      llvm::MDString::get(generator->llvmContext, "var_nature"));
  variableAlloca->setMetadata(llvm::StringRef(isVariable ? "var" : "const"),
                              varNatureMD);
  generator->builder.SetInsertPoint(&*oldInsertPoint);
  llvm::StoreInst *storeValue =
      generator->builder.CreateStore(genValue, variableAlloca, false);
  return storeValue;
}

qat::AST::NodeType qat::AST::LocalDeclaration::nodeType() {
  return qat::AST::NodeType::localDeclaration;
}