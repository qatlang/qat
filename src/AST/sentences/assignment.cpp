#include "./assignment.hpp"

namespace qat {
namespace AST {

Assignment::Assignment(Expression *_lhs, Expression *_value,
                       utils::FilePlacement _filePlacement)
    : lhs(_lhs), value(_value), Sentence(_filePlacement) {}

IR::Value *Assignment::emit(qat::IR::Context *ctx) {
  // lhs->setExpectedKind(ExpressionKind::assignable);
  // auto lhs_val = lhs->emit(ctx);
  // bool is_reference = false;
  // llvm::Type *lhs_type = nullptr;
  // if (llvm::isa<llvm::Instruction>(lhs_val)) {
  //   bool isAlloca = llvm::isa<llvm::AllocaInst>(lhs_val);
  //   auto inst = isAlloca ? llvm::dyn_cast<llvm::AllocaInst>(lhs_val)
  //                        : llvm::dyn_cast<llvm::Instruction>(lhs_val);
  //   if (utils::Variability::get(inst)) {
  //     if (isAlloca) {
  //       lhs_type =
  //           llvm::dyn_cast<llvm::AllocaInst>(lhs_val)->getAllocatedType();
  //     }
  //     is_reference = utils::PointerKind::is_reference(inst);
  //   } else {
  //     Errors::AST1(lhs_val->getName().str(), lhs->file_placement);
  //   }
  // } else if (llvm::isa<llvm::GlobalVariable>(lhs_val)) {
  //   auto gvar = llvm::dyn_cast<llvm::GlobalVariable>(lhs_val);
  //   if (utils::Variability::get(gvar)) {
  //     lhs_type = gvar->getValueType();
  //     is_reference = utils::PointerKind::is_reference(gvar);
  //   } else {
  //     Errors::AST2(true, lhs_val->getName().str(), file_placement);
  //   }
  // }
  // if (!lhs_type) {
  //   lhs_type = lhs_val->getType();
  // }
  // auto gen_val = value->emit(ctx);
  // if (is_reference
  //         ? (llvm::dyn_cast<llvm::PointerType>(lhs_type)->getElementType() !=
  //            gen_val->getType())
  //         : lhs_type != gen_val->getType()) {
  //   Errors::AST0(
  //       utils::llvmTypeToName(
  //           is_reference
  //               ?
  //               llvm::dyn_cast<llvm::PointerType>(lhs_type)->getElementType()
  //               : lhs_type),
  //       utils::llvmTypeToName(gen_val->getType()), file_placement);
  // }
  // // TODO - Support copy and move semantics
  // return ctx->builder.CreateStore(gen_val, lhs_val, false);
}

void Assignment::emitCPP(backend::cpp::File &file, bool isHeader) const {
  lhs->emitCPP(file, isHeader);
  file += " = ";
  value->emitCPP(file, isHeader);
  file += ";\n";
}

backend::JSON Assignment::toJSON() const {
  return backend::JSON()
      ._("nodeType", "assignment")
      ._("lhs", lhs->toJSON())
      ._("rhs", value->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat
