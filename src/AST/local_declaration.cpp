#include "./local_declaration.hpp"

namespace qat {
namespace AST {

LocalDeclaration::LocalDeclaration(std::optional<QatType> _type,
                                   std::string _name, Expression _value,
                                   bool _variability,
                                   utils::FilePlacement _filePlacement)
    : type(_type), name(_name), value(_value), variability(_variability),
      Sentence(_filePlacement) {}

llvm::Value *LocalDeclaration::generate(qat::IR::Generator *generator) {
  bool is_reference = type.has_value()
                          ? (type.value().typeKind() == TypeKind::reference)
                          : false;
  auto gen_value = value.generate(generator);
  if (type.has_value()) {
    auto decl_ty = type.value().generate(generator);
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
        gen_value =
            generator->builder.CreateCall(conv_fn->getFunctionType(), conv_fn,
                                          {args}, "conversion_call", nullptr);
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
  auto origin_bb = generator->builder.GetInsertBlock();
  generator->builder.SetInsertPoint(&*insert_p);
  auto var_alloca = generator->builder.CreateAlloca(
      (type.has_value() ? type.value().generate(generator)
                        : gen_value->getType()),
      0, nullptr, name);
  if (var_alloca->getAllocatedType()->isPointerTy()) {
    var_alloca->setMetadata(
        "pointer_type",
        llvm::MDNode::get(
            generator->llvmContext,
            {llvm::MDString::get(generator->llvmContext,
                                 is_reference ? "reference" : "pointer")}));
  }
  set_origin_block(generator->llvmContext, var_alloca, origin_bb);
  utils::Variability::set(generator->llvmContext, var_alloca, variability);
  generator->builder.SetInsertPoint(origin_bb);
  auto storeValue =
      generator->builder.CreateStore(gen_value, var_alloca, false);
  return storeValue;
}

void LocalDeclaration::set_origin_block(llvm::LLVMContext &ctx,
                                        llvm::AllocaInst *alloca,
                                        llvm::BasicBlock *bb) {

  alloca->setMetadata(
      "origin_block",
      llvm::MDNode::get(ctx, llvm::MDString::get(ctx, bb->getName().str())));
}

} // namespace AST
} // namespace qat