#include "./member_access.hpp"

namespace qat::ast {

MemberAccess::MemberAccess(Expression *_instance, bool _isPointerAccess,
                           String _name, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), instance(_instance),
      name(std::move(_name)), isPointer(_isPointerAccess) {}

IR::Value *MemberAccess::emit(IR::Context *ctx) {
  SHOW("Member variable emitting")
  auto *inst     = instance->emit(ctx);
  auto *instType = inst->getType();
  if (instType->isReference()) {
    instType = instType->asReference()->getSubType();
  }
  if (instType->isArray()) {
    if (name == "length") {
      return new IR::Value(
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx),
                                 instType->asArray()->getLength()),
          // NOLINTNEXTLINE(readability-magic-numbers)
          IR::IntegerType::get(64u, ctx->llctx), false, IR::Nature::pure);
    } else {
      ctx->Error("Invalid name for member access " + ctx->highlightError(name),
                 fileRange);
    }
  } else {
    ctx->Error("Member access for this type is not supported", fileRange);
  }
  return nullptr;
}

nuo::Json MemberAccess::toJson() const {
  return nuo::Json()
      ._("nodeType", "memberVariable")
      ._("instance", instance->toJson())
      ._("isPointerAccess", isPointer)
      ._("member", name)
      ._("fileRange", fileRange);
}

} // namespace qat::ast