#include "./local_declaration.hpp"
#include "../show.hpp"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace AST {

LocalDeclaration::LocalDeclaration(QatType *_type, std::string _name,
                                   Expression *_value, bool _variability,
                                   utils::FilePlacement _filePlacement)
    : type(_type), name(_name), value(_value), variability(_variability),
      Sentence(_filePlacement) {}

llvm::Value *LocalDeclaration::emit(qat::IR::Generator *generator) {
  bool is_reference = type ? (type->typeKind() == TypeKind::reference) : false;
  auto gen_value = value->emit(generator);
  if (type) {
    auto decl_ty = type->emit(generator);
    if ((decl_ty->isPointerTy()) &&
        (value->nodeType() == NodeType::nullPointer)) {
      gen_value = generator->builder.CreatePointerCast(
          gen_value, decl_ty, "implcitNullPointerCast");
    } else if (decl_ty != gen_value->getType()) {
      generator->throw_error("The type of the entity being declared does not "
                             "match with the value provided",
                             file_placement);
    }
  }
  auto function = generator->builder.GetInsertBlock()->getParent();
  auto e_block = function->getEntryBlock().begin()->getParent();
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
  }
  SHOW("Getting origin block")
  auto origin_bb = generator->builder.GetInsertBlock();
  SHOW("Setting insert point")
  generator->builder.SetInsertPoint(e_block);
  SHOW("Insert point set")
  auto var_alloca = new llvm::AllocaInst(
      (type ? type->emit(generator) : gen_value->getType()), 0u, name, e_block);
  SHOW("Alloca created")
  if (var_alloca->getAllocatedType()->isPointerTy()) {
    utils::PointerKind::set(generator->llvmContext, var_alloca, is_reference);
    SHOW("Pointer kind handled")
  }
  SHOW("Setting origin block to alloca")
  set_origin_block(generator->llvmContext, var_alloca, origin_bb);
  utils::Variability::set(generator->llvmContext, var_alloca, variability);
  generator->builder.SetInsertPoint(origin_bb);
  SHOW("Going back to origin block")
  auto storeValue =
      generator->builder.CreateStore(gen_value, var_alloca, false);
  SHOW("Store created")
  return storeValue;
}

void LocalDeclaration::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    if (variability) {
      file += "const ";
    }
    if (type) {
      type->emitCPP(file, isHeader);
    } else {
      file += "auto ";
    }
    file += name;
    file += " = ";
    value->emitCPP(file, isHeader);
    file += ";\n";
  }
}

void LocalDeclaration::set_origin_block(llvm::LLVMContext &ctx,
                                        llvm::AllocaInst *alloca,
                                        llvm::BasicBlock *bb) {

  alloca->setMetadata(
      "origin_block",
      llvm::MDNode::get(ctx, llvm::MDString::get(ctx, bb->getName().str())));
}

backend::JSON LocalDeclaration::toJSON() const {
  return backend::JSON()
      ._("nodeType", "localDeclaration")
      ._("name", name)
      ._("isVariable", variability)
      ._("hasType", (type != nullptr))
      ._("type", (type != nullptr) ? type->toJSON() : backend::JSON())
      ._("value", value->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat