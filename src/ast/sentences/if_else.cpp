#include "./if_else.hpp"
#include "../../IR/control_flow.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

IfElse::IfElse(Vec<Pair<Expression*, Vec<Sentence*>>> _chain, Maybe<Vec<Sentence*>> _else, utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), chain(std::move(_chain)), elseCase(std::move(_else)) {}

Pair<bool, usize> IfElse::trueKnownValue(usize ind) const {
  for (usize i = 0; i < ind; i++) {
    if (knownVals.at(i).has_value() && knownVals.at(i).value()) {
      return {true, i};
    }
  }
  return {false, ind};
}

bool IfElse::getKnownValue(usize ind) const {
  if (knownVals.at(ind).has_value()) {
    return knownVals.at(ind).value();
  }
  return false;
}

bool IfElse::hasValueAt(usize ind) const { return knownVals.at(ind).has_value(); }

bool IfElse::isFalseTill(usize ind) const {
  for (usize i = 0; i < ind; i++) {
    if (knownVals.at(i).has_value()) {
      if (knownVals.at(i).value()) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

IR::Value* IfElse::emit(IR::Context* ctx) {
  auto* restBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
  for (usize i = 0; i < chain.size(); i++) {
    const auto& section = chain.at(i);
    auto*       exp     = section.first->emit(ctx);
    auto*       expTy   = exp->getType();
    if (expTy->isReference()) {
      expTy = expTy->asReference()->getSubType();
    }
    if (!expTy->isUnsignedInteger()) {
      ctx->Error("Expression in if sentence should be of unsigned integer type", section.first->fileRange);
    }
    if (exp->isConstVal()) {
      auto condConstVal = *(llvm::dyn_cast<llvm::ConstantInt>(exp->asConst()->getLLVM())->getValue().getRawData());
      knownVals.push_back(condConstVal == 1u);
    } else {
      knownVals.push_back(None);
    }
    if (!trueKnownValue(i).first) {
      if (hasValueAt(i) && isFalseTill(i)) {
        if (getKnownValue(i)) {
          auto* trueBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
          (void)IR::addBranch(ctx->builder, trueBlock->getBB());
          emitSentences(section.second, ctx);
          (void)IR::addBranch(ctx->builder, restBlock->getBB());
        }
      } else {
        auto*      trueBlock  = new IR::Block(ctx->fn, ctx->fn->getBlock());
        IR::Block* falseBlock = nullptr;
        if (exp->isImplicitPointer() || exp->isReference()) {
          exp = new IR::Value(ctx->builder.CreateLoad(expTy->getLLVMType(), exp->getLLVM()), expTy, false,
                              IR::Nature::temporary);
        }
        if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
          falseBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
          ctx->builder.CreateCondBr(exp->getLLVM(), trueBlock->getBB(), falseBlock->getBB());
        } else {
          ctx->builder.CreateCondBr(exp->getLLVM(), trueBlock->getBB(), restBlock->getBB());
        }
        trueBlock->setActive(ctx->builder);
        emitSentences(section.second, ctx);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
        if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
          // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
          falseBlock->setActive(ctx->builder);
        }
      }
    }
  }
  if (elseCase.has_value()) {
    if (hasAnyKnownValue()) {
      if (isFalseTill(knownVals.size())) {
        auto* elseBlock = new IR::Block(ctx->fn, ctx->fn->getBlock());
        elseBlock->setActive(ctx->builder);
        emitSentences(elseCase.value(), ctx);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
      }
    } else {
      emitSentences(elseCase.value(), ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
    }
  } else {
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
  }
  restBlock->setActive(ctx->builder);
  return nullptr;
}

Json IfElse::toJson() const {
  Vec<JsonValue> _chain;
  for (const auto& elem : chain) {
    Vec<JsonValue> snts;
    for (auto* snt : elem.second) {
      snts.push_back(snt->toJson());
    }
    _chain.push_back(Json()._("expression", elem.first->toJson())._("sentences", snts));
  }
  Vec<JsonValue> elseSnts;
  if (elseCase.has_value()) {
    for (auto* snt : elseCase.value()) {
      elseSnts.push_back(snt->toJson());
    }
  }
  return Json()
      ._("nodeType", "ifElse")
      ._("chain", _chain)
      ._("hasElse", (elseCase.has_value()))
      ._("else", elseSnts)
      ._("fileRange", fileRange);
}

} // namespace qat::ast