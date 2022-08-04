#include "./void.hpp"

namespace qat::IR {

TypeKind VoidType::typeKind() const { return TypeKind::Void; }

String VoidType::toString() const { return "void"; }

llvm::Type *VoidType::emitLLVM(llvmHelper &help) const {
  return llvm::Type::getVoidTy(help.llctx);
}

void VoidType::emitCPP(cpp::File &file) const { file << "void"; }

nuo::Json VoidType::toJson() const { return nuo::Json()._("typeKind", "void"); }

} // namespace qat::IR