#include "./if_else.hpp"
#include "../../IR/control_flow.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

Pair<bool, usize> IfElse::trueKnownValueBefore(usize ind) const {
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
  for (usize i = 0; (i < ind) && (i < knownVals.size()); i++) {
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
  auto* restBlock = new IR::Block(ctx->getActiveFunction(), nullptr);
  restBlock->linkPrevBlock(ctx->getActiveFunction()->getBlock());
  for (usize i = 0; i < chain.size(); i++) {
    const auto& section = chain.at(i);
    auto*       exp     = std::get<0>(section)->emit(ctx);
    auto*       expTy   = exp->getType();
    if (expTy->isReference()) {
      expTy = expTy->asReference()->getSubType();
    }
    if (!expTy->isBool()) {
      ctx->Error("Condition in an " + ctx->highlightError("if") + " block should be of " + ctx->highlightError("bool") +
                     " type",
                 std::get<0>(section)->fileRange);
    }
    if (exp->isPrerunValue()) {
      SHOW("Is const condition in if-else")
      auto condConstVal = *(llvm::dyn_cast<llvm::ConstantInt>(exp->asPrerun()->getLLVM())->getValue().getRawData());
      knownVals.push_back(condConstVal == 1u);
    } else {
      knownVals.push_back(None);
    }
    if (!trueKnownValueBefore(i).first) {
      if (hasValueAt(i) && isFalseTill(i)) {
        if (getKnownValue(i)) {
          auto* trueBlock = new IR::Block(ctx->getActiveFunction(), ctx->getActiveFunction()->getBlock());
          trueBlock->setFileRange(std::get<2>(section));
          (void)IR::addBranch(ctx->builder, trueBlock->getBB());
          trueBlock->setActive(ctx->builder);
          emitSentences(std::get<1>(section), ctx);
          trueBlock->destroyLocals(ctx);
          (void)IR::addBranch(ctx->builder, restBlock->getBB());
          break;
        }
      } else {
        auto* trueBlock = new IR::Block(ctx->getActiveFunction(), ctx->getActiveFunction()->getBlock());
        trueBlock->setFileRange(std::get<2>(section));
        IR::Block* falseBlock = nullptr;
        if (exp->isImplicitPointer() || exp->isReference()) {
          exp = new IR::Value(ctx->builder.CreateLoad(expTy->getLLVMType(), exp->getLLVM()), expTy, false,
                              IR::Nature::temporary);
        }
        if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
          falseBlock = new IR::Block(ctx->getActiveFunction(), ctx->getActiveFunction()->getBlock());
          if (i == (chain.size() - 1)) {
            falseBlock->setFileRange(elseCase.value().second);
          } else {
            falseBlock->setFileRange(std::get<2>(chain.at(i + 1)));
          }
          ctx->builder.CreateCondBr(exp->getLLVM(), trueBlock->getBB(), falseBlock->getBB());
        } else {
          ctx->builder.CreateCondBr(exp->getLLVM(), trueBlock->getBB(), restBlock->getBB());
        }
        trueBlock->setActive(ctx->builder);
        emitSentences(std::get<1>(section), ctx);
        trueBlock->destroyLocals(ctx);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
        if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
          // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
          falseBlock->setActive(ctx->builder);
        }
      }
    }
  }
  if (elseCase.has_value()) {
    if (hasAnyKnownValue() && isFalseTill(knownVals.size())) {
      auto* elseBlock = new IR::Block(ctx->getActiveFunction(), ctx->getActiveFunction()->getBlock());
      (void)IR::addBranch(ctx->builder, elseBlock->getBB());
      elseBlock->setActive(ctx->builder);
      emitSentences(elseCase.value().first, ctx);
      elseBlock->destroyLocals(ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
    } else if (!hasAnyKnownValue() || (hasAnyKnownValue() && !trueKnownValueBefore(knownVals.size()).first)) {
      emitSentences(elseCase.value().first, ctx);
      ctx->getActiveFunction()->getBlock()->destroyLocals(ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
    }
  } else {
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
  }
  restBlock->setActive(ctx->builder);
  if (ctx->getActiveFunction()->hasDefinitionRange()) {
    restBlock->setFileRange(
        FileRange(fileRange.file, fileRange.end, ctx->getActiveFunction()->getDefinitionRange().end));
  }
  return nullptr;
}

Json IfElse::toJson() const {
  Vec<JsonValue> _chain;
  for (const auto& elem : chain) {
    Vec<JsonValue> snts;
    for (auto* snt : std::get<1>(elem)) {
      snts.push_back(snt->toJson());
    }
    _chain.push_back(
        Json()._("expression", std::get<0>(elem)->toJson())._("sentences", snts)._("fileRange", std::get<2>(elem)));
  }
  Json           elseJson;
  Vec<JsonValue> elseSnts;
  if (elseCase.has_value()) {
    for (auto* snt : elseCase.value().first) {
      elseSnts.push_back(snt->toJson());
    }
    elseJson._("sentences", elseSnts);
    elseJson._("fileRange", elseCase.value().second);
  }

  return Json()
      ._("nodeType", "ifElse")
      ._("chain", _chain)
      ._("hasElse", (elseCase.has_value()))
      ._("else", elseJson)
      ._("fileRange", fileRange);
}

} // namespace qat::ast