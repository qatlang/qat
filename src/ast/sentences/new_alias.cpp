#include "./new_alias.hpp"

namespace qat::ast {

NewAlias::NewAlias(String _name, Expression *_exp, utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), name(std::move(_name)), exp(_exp) {}

IR::Value *NewAlias::emit(IR::Context *ctx) {
  auto *block = ctx->fn->getBlock();
  if (!block->hasAlias(name)) {
    block->addAlias(name, exp->emit(ctx));
  } else {
    ctx->Error(
        "There already exists another alias with the same name in this scope",
        fileRange);
  }
  return nullptr;
}

nuo::Json NewAlias::toJson() const {
  return nuo::Json()
      ._("nodeType", "newAlias")
      ._("name", name)
      ._("expression", exp->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast