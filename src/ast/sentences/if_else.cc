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

ir::Value* IfElse::emit(EmitCtx* ctx) {
  auto* restBlock = new ir::Block(ctx->get_fn(), nullptr);
  restBlock->link_previous_block(ctx->get_fn()->get_block());
  for (usize i = 0; i < chain.size(); i++) {
    const auto& section = chain.at(i);
    auto*       exp     = std::get<0>(section)->emit(ctx);
    auto*       expTy   = exp->get_ir_type();
    if (expTy->is_reference()) {
      expTy = expTy->as_reference()->get_subtype();
    }
    if (!expTy->is_bool()) {
      ctx->Error("Condition in an " + ctx->color("if") + " block should be of " + ctx->color("bool") + " type",
                 std::get<0>(section)->fileRange);
    }
    if (exp->is_prerun_value()) {
      SHOW("Is const condition in if-else")
      auto condConstVal = *(llvm::dyn_cast<llvm::ConstantInt>(exp->as_prerun()->get_llvm())->getValue().getRawData());
      knownVals.push_back(condConstVal == 1u);
    } else {
      knownVals.push_back(None);
    }
    if (!trueKnownValueBefore(i).first) {
      if (hasValueAt(i) && isFalseTill(i)) {
        if (getKnownValue(i)) {
          auto* trueBlock = new ir::Block(ctx->get_fn(), ctx->get_fn()->get_block());
          trueBlock->set_file_range(std::get<2>(section));
          (void)ir::add_branch(ctx->irCtx->builder, trueBlock->get_bb());
          trueBlock->set_active(ctx->irCtx->builder);
          emit_sentences(std::get<1>(section), ctx);
          trueBlock->destroy_locals(ctx);
          (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
          break;
        }
      } else {
        auto* trueBlock = new ir::Block(ctx->get_fn(), ctx->get_fn()->get_block());
        trueBlock->set_file_range(std::get<2>(section));
        ir::Block* falseBlock = nullptr;
        if (exp->is_ghost_reference() || exp->is_reference()) {
          exp = ir::Value::get(ctx->irCtx->builder.CreateLoad(expTy->get_llvm_type(), exp->get_llvm()), expTy, false);
        }
        if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
          falseBlock = new ir::Block(ctx->get_fn(), ctx->get_fn()->get_block());
          if (i == (chain.size() - 1)) {
            falseBlock->set_file_range(elseCase.value().second);
          } else {
            falseBlock->set_file_range(std::get<2>(chain.at(i + 1)));
          }
          ctx->irCtx->builder.CreateCondBr(exp->get_llvm(), trueBlock->get_bb(), falseBlock->get_bb());
        } else {
          ctx->irCtx->builder.CreateCondBr(exp->get_llvm(), trueBlock->get_bb(), restBlock->get_bb());
        }
        trueBlock->set_active(ctx->irCtx->builder);
        emit_sentences(std::get<1>(section), ctx);
        trueBlock->destroy_locals(ctx);
        (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
        if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
          // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
          falseBlock->set_active(ctx->irCtx->builder);
        }
      }
    }
  }
  if (elseCase.has_value()) {
    if (hasAnyKnownValue() && isFalseTill(knownVals.size())) {
      auto* elseBlock = new ir::Block(ctx->get_fn(), ctx->get_fn()->get_block());
      (void)ir::add_branch(ctx->irCtx->builder, elseBlock->get_bb());
      elseBlock->set_active(ctx->irCtx->builder);
      emit_sentences(elseCase.value().first, ctx);
      elseBlock->destroy_locals(ctx);
      (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
    } else if (!hasAnyKnownValue() || (hasAnyKnownValue() && !trueKnownValueBefore(knownVals.size()).first)) {
      emit_sentences(elseCase.value().first, ctx);
      ctx->get_fn()->get_block()->destroy_locals(ctx);
      (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
    }
  } else {
    (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
  }
  restBlock->set_active(ctx->irCtx->builder);
  if (ctx->get_fn()->has_definition_range()) {
    restBlock->set_file_range(FileRange(fileRange.file, fileRange.end, ctx->get_fn()->get_definition_range().end));
  }
  return nullptr;
}

Json IfElse::to_json() const {
  Vec<JsonValue> _chain;
  for (const auto& elem : chain) {
    Vec<JsonValue> snts;
    for (auto* snt : std::get<1>(elem)) {
      snts.push_back(snt->to_json());
    }
    _chain.push_back(
        Json()._("expression", std::get<0>(elem)->to_json())._("sentences", snts)._("fileRange", std::get<2>(elem)));
  }
  Json           elseJson;
  Vec<JsonValue> elseSnts;
  if (elseCase.has_value()) {
    for (auto* snt : elseCase.value().first) {
      elseSnts.push_back(snt->to_json());
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
