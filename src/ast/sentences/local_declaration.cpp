#include "./local_declaration.hpp"
#include "../../show.hpp"

namespace qat::ast {

LocalDeclaration::LocalDeclaration(QatType *_type, String _name,
                                   Expression *_value, bool _variability,
                                   utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), type(_type), name(std::move(_name)),
      value(_value), variability(_variability) {}

IR::Value *LocalDeclaration::emit(IR::Context *ctx) {
  auto *block = ctx->fn->getBlock();
  if (block->hasValue(name)) {
    ctx->Error("A local value named " + ctx->highlightError(name) +
                   " already exists in this scope. Please change name of this "
                   "declaration or check the logic",
               fileRange);
  }
  IR::Value *expVal = nullptr;
  if (value) {
    expVal = value->emit(ctx);
    SHOW("Type of value to be assigned to local value "
         << name << " is " << expVal->getType()->toString())
  }
  IR::QatType *declType = nullptr;
  if (type) {
    declType = type->emit(ctx);
    if (value &&
        ((declType->isReference() && !expVal->isReference())
             ? !declType->asReference()->getSubType()->isSame(expVal->getType())
             : !declType->isSame(expVal->getType()))) {
      ctx->Error("Type of the local value " + ctx->highlightError(name) +
                     " does not match the expression to be assigned",
                 fileRange);
    }
  } else {
    SHOW("No type for decl. Getting type from value")
    if (expVal) {
      SHOW("Getting type from expression")
      declType = expVal->getType();
    } else {
      ctx->Error("Type inference for declarations require a value", fileRange);
    }
  }
  auto *newValue = block->newValue(name, declType, variability);
  if (expVal) {
    SHOW("Creating store")
    if ((expVal->isImplicitPointer() && !declType->isReference()) ||
        (expVal->getType()->isReference() && declType->isReference())) {
      expVal->loadImplicitPointer(ctx->builder);
    }
    ctx->builder.CreateStore(expVal->getLLVM(), newValue->getAlloca());
    SHOW("llvm::StoreInst created")
  }
  return nullptr;
}

nuo::Json LocalDeclaration::toJson() const {
  return nuo::Json()
      ._("nodeType", "localDeclaration")
      ._("name", name)
      ._("isVariable", variability)
      ._("hasType", (type != nullptr))
      ._("type", (type != nullptr) ? type->toJson() : nuo::Json())
      ._("value", value->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast