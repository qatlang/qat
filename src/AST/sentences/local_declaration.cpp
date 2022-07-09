#include "./local_declaration.hpp"
#include "../../show.hpp"
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

IR::Value *LocalDeclaration::emit(qat::IR::Context *ctx) {
  // TODO - Implement this
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