#include "./local_declaration.hpp"
#include "../../show.hpp"

namespace qat::AST {

LocalDeclaration::LocalDeclaration(QatType *_type, std::string _name,
                                   Expression *_value, bool _variability,
                                   utils::FilePlacement _filePlacement)
    : Sentence(_filePlacement), type(_type), name(_name), value(_value),
      variability(_variability) {}

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

nuo::Json LocalDeclaration::toJson() const {
  return nuo::Json()
      ._("nodeType", "localDeclaration")
      ._("name", name)
      ._("isVariable", variability)
      ._("hasType", (type != nullptr))
      ._("type", (type != nullptr) ? type->toJson() : nuo::Json())
      ._("value", value->toJson())
      ._("filePlacement", file_placement);
}

} // namespace qat::AST