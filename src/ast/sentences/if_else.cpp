#include "./if_else.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

IfElse::IfElse(Vec<Pair<Expression *, Vec<Sentence *>>> _chain,
               Maybe<Vec<Sentence *>> _else, utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), chain(std::move(_chain)),
      elseCase(std::move(_else)) {}

IR::Value *IfElse::emit(IR::Context *ctx) {
  auto *restBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
  for (usize i = 0; i < chain.size(); i++) {
    const auto &section = chain.at(i);
    auto       *exp     = section.first->emit(ctx);
    if (!exp->getType()->isUnsignedInteger()) {
      ctx->Error("Expression in if sentence should be of unsigned integer type",
                 section.first->fileRange);
    }
    auto      *trueBlock  = new IR::Block(ctx->fn, ctx->fn->getBlock());
    IR::Block *falseBlock = nullptr;
    if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
      falseBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
      ctx->builder.CreateCondBr(exp->getLLVM(), trueBlock->getBB(),
                                falseBlock->getBB());
    } else {
      ctx->builder.CreateCondBr(exp->getLLVM(), trueBlock->getBB(),
                                restBlock->getBB());
    }
    trueBlock->setActive(ctx->builder);
    emitSentences(section.second, ctx);
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      falseBlock->setActive(ctx->builder);
    }
  }
  if (elseCase.has_value()) {
    emitSentences(elseCase.value(), ctx);
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
  }
  restBlock->setActive(ctx->builder);
  return nullptr;
}

nuo::Json IfElse::toJson() const {
  Vec<nuo::JsonValue> _chain;
  for (const auto &elem : chain) {
    Vec<nuo::JsonValue> snts;
    for (auto *snt : elem.second) {
      snts.push_back(snt->toJson());
    }
    _chain.push_back(
        nuo::Json()._("expression", elem.first->toJson())._("sentences", snts));
  }
  Vec<nuo::JsonValue> elseSnts;
  if (elseCase.has_value()) {
    for (auto *snt : elseCase.value()) {
      elseSnts.push_back(snt->toJson());
    }
  }
  return nuo::Json()
      ._("nodeType", "ifElse")
      ._("chain", _chain)
      ._("hasElse", (elseCase.has_value()))
      ._("else", elseSnts)
      ._("fileRange", fileRange);
}

} // namespace qat::ast