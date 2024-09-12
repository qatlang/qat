#include "./match.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

MixOrChoiceMatchValue* MatchValue::asMixOrChoice() { return (MixOrChoiceMatchValue*)this; }
ExpressionMatchValue*  MatchValue::asExp() { return (ExpressionMatchValue*)this; }

MixOrChoiceMatchValue::MixOrChoiceMatchValue(Identifier _name, Maybe<Identifier> _valueName, bool _isVar)
    : name(std::move(_name)), valueName(std::move(_valueName)), isVar(_isVar) {}

Identifier MixOrChoiceMatchValue::get_name() const { return name; }

bool MixOrChoiceMatchValue::hasValueName() const { return valueName.has_value(); }

bool MixOrChoiceMatchValue::is_variable() const { return isVar; }

Identifier MixOrChoiceMatchValue::getValueName() const { return valueName.value(); }

Json MixOrChoiceMatchValue::to_json() const {
  return Json()
      ._("matchValueType", "mix")
      ._("name", name)
      ._("hasValue", valueName.has_value())
      ._("valueName", valueName.has_value() ? valueName.value() : JsonValue());
}

Expression* ExpressionMatchValue::getExpression() const { return exp; }

Json ExpressionMatchValue::to_json() const {
  SHOW("Expression node type is: " << (int)exp->nodeType())
  return Json()._("matchValueType", "expression")._("expression", exp->to_json());
}

CaseResult::CaseResult(Maybe<bool> _result, bool _areAllConst) : result(_result), areAllConstant(_areAllConst) {}

ir::Value* Match::emit(EmitCtx* ctx) {
  auto* expEmit       = candidate->emit(ctx);
  auto* expTy         = expEmit->get_ir_type();
  bool  isExpVariable = false;
  if (expTy->is_reference()) {
    expEmit->load_ghost_pointer(ctx->irCtx->builder);
    isExpVariable = expTy->as_reference()->isSubtypeVariable();
    expTy         = expTy->as_reference()->get_subtype();
  } else {
    isExpVariable = expEmit->is_variable();
  }
  auto* curr      = ctx->get_fn()->get_block();
  auto* restBlock = new ir::Block(ctx->get_fn(), curr->get_parent());
  restBlock->link_previous_block(curr);
  bool elseNotRequired = false;
  if (expTy->is_mix()) {
    auto*           mTy = expTy->as_mix();
    Vec<Identifier> mentionedFields;
    ir::Block*      falseBlock = nullptr;
    for (usize i = 0; i < chain.size(); i++) {
      const auto&       section = chain.at(i);
      Vec<llvm::Value*> conditions;
      for (usize k = 0; k < section.first.size(); k++) {
        if (section.first.at(k)->getType() != MatchType::mixOrChoice) {
          ctx->Error("Expected the name of a variant of the mix type " + ctx->color(expTy->as_mix()->get_full_name()),
                     section.first.at(k)->getMainRange());
        }
        auto* uMatch = section.first.at(k)->asMixOrChoice();
        if (uMatch->is_variable()) {
          if (!isExpVariable) {
            ctx->Error("The expression being matched does not possess variability and "
                       "hence the matched "
                       "case cannot use a variable reference to the subtype value",
                       uMatch->getValueName().range);
          }
        }
        auto subRes = mTy->has_variant_with_name(uMatch->get_name().value);
        if (!subRes.first) {
          ctx->Error("No field named " + ctx->color(uMatch->get_name().value) + " in mix type " +
                         ctx->color(mTy->get_full_name()),
                     uMatch->get_name().range);
        }
        if (uMatch->hasValueName()) {
          if (section.first.size() > 1) {
            ctx->Error(
                "Multiple values are provided to be matched for this case, and hence the matched value cannot be requested for use",
                uMatch->getMainRange());
          }
        }
        if (!subRes.second && uMatch->hasValueName()) {
          ctx->Error("Sub-field " + ctx->color(uMatch->get_name().value) + " of mix type " +
                         ctx->color(mTy->get_full_name()) +
                         " does not have a type associated with it, and hence you "
                         "cannot use a name for the value here",
                     uMatch->getValueName().range);
        }
        // NOTE - When expressions are supported for mix types, take care of this
        for (usize mixValIdx = k + 1; mixValIdx < section.first.size(); mixValIdx++) {
          if (uMatch->get_name().value == chain.at(i).first.at(mixValIdx)->asMixOrChoice()->get_name().value) {
            ctx->Error("Repeating subfield of the mix type ",
                       chain.at(i).first.at(mixValIdx)->asMixOrChoice()->get_name().range);
          }
        }
        for (usize j = i + 1; j < chain.size(); j++) {
          for (auto* matchVal : chain.at(j).first) {
            if (uMatch->get_name().value == matchVal->asMixOrChoice()->get_name().value) {
              ctx->Error("Repeating subfield of the mix type " + ctx->color(mTy->get_full_name()),
                         matchVal->asMixOrChoice()->get_name().range);
            }
          }
        }
        mentionedFields.push_back(uMatch->get_name());
        conditions.push_back(ctx->irCtx->builder.CreateICmpEQ(
            expEmit->is_value()
                ? ctx->irCtx->builder.CreateExtractValue(expEmit->get_llvm(), 0u)
                : ctx->irCtx->builder.CreateLoad(
                      llvm::Type::getIntNTy(ctx->irCtx->llctx, mTy->get_tag_bitwidth()),
                      ctx->irCtx->builder.CreateStructGEP(mTy->get_llvm_type(), expEmit->get_llvm(), 0)),
            llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->irCtx->llctx, mTy->get_tag_bitwidth()),
                                   mTy->get_index_of(uMatch->get_name().value))));
      }
      auto* trueBlock = new ir::Block(ctx->get_fn(), curr);
      falseBlock      = nullptr;
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        falseBlock = new ir::Block(ctx->get_fn(), curr);
        if (conditions.size() == 1) {
          ctx->irCtx->builder.CreateCondBr(conditions.front(), trueBlock->get_bb(), falseBlock->get_bb());
        } else {
          ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateOr(conditions), trueBlock->get_bb(),
                                           falseBlock->get_bb());
        }
      } else {
        if (conditions.size() == 1) {
          ctx->irCtx->builder.CreateCondBr(conditions.front(), trueBlock->get_bb(), restBlock->get_bb());
        } else {
          ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateOr(conditions), trueBlock->get_bb(),
                                           restBlock->get_bb());
        }
      }
      trueBlock->set_active(ctx->irCtx->builder);
      SHOW("True block name is: " << trueBlock->get_name())
      if ((section.first.size() == 1) && section.first.front()->asMixOrChoice()->hasValueName()) {
        auto* uMatch = section.first.front()->asMixOrChoice();
        SHOW("Match case has value name: " << uMatch->getValueName().value)
        if (trueBlock->has_value(uMatch->getValueName().value)) {
          SHOW("Local entity with match case value name exists")
          ctx->Error("Local entity named " + ctx->color(uMatch->getValueName().value) + " exists already in this scope",
                     uMatch->getValueName().range);
        } else {
          SHOW("Creating local entity for match case value named: " << uMatch->getValueName().value)
          auto* loc = trueBlock->new_value(
              uMatch->getValueName().value,
              expEmit->is_value()
                  ? mTy->get_variant_with_name(uMatch->get_name().value)
                  : ir::ReferenceType::get(uMatch->is_variable(), mTy->get_variant_with_name(uMatch->get_name().value),
                                           ctx->irCtx),
              false, uMatch->getValueName().range);
          SHOW("Local Entity for match case created")
          ctx->irCtx->builder.CreateStore(
              expEmit->is_value()
                  ? ctx->irCtx->builder.CreateExtractValue(expEmit->get_llvm(), {1u})
                  : ctx->irCtx->builder.CreatePointerCast(
                        ctx->irCtx->builder.CreateStructGEP(mTy->get_llvm_type(), expEmit->get_llvm(), 1),
                        mTy->get_variant_with_name(uMatch->get_name().value)->get_llvm_type()->getPointerTo()),
              loc->get_alloca());
        }
      }
      emit_sentences(section.second, ctx);
      trueBlock->destroy_locals(ctx);
      (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        falseBlock->set_active(ctx->irCtx->builder);
      }
    }
    Vec<Identifier> missingFields;
    mTy->get_missing_names(mentionedFields, missingFields);
    if (missingFields.empty()) {
      if (elseCase.has_value()) {
        ctx->irCtx->Warning("All possible variants of the mix type are already "
                            "mentioned in the match sentence. No need to use else case "
                            "for the match statement",
                            elseCase.value().second);
      }
      elseNotRequired = true;
      if (falseBlock) {
        falseBlock->set_active(ctx->irCtx->builder);
        (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
      }
    } else {
      if (!elseCase.has_value()) {
        ctx->Error("Not all possible variants of the mix type are provided. "
                   "Please add the else case to handle all missing variants",
                   fileRange);
      }
    }
  } else if (expTy->is_choice()) {
    auto* chTy = expTy->as_choice();
    SHOW("Got choice type")
    llvm::Value* choiceVal;
    if (expEmit->is_reference() || expEmit->is_ghost_pointer()) {
      choiceVal = ctx->irCtx->builder.CreateLoad(chTy->get_llvm_type(), expEmit->get_llvm());
    } else {
      choiceVal = expEmit->get_llvm();
    }
    Vec<Identifier> mentionedFields;
    ir::Block*      falseBlock = nullptr;
    for (usize i = 0; i < chain.size(); i++) {
      const auto&       section = chain.at(i);
      Vec<llvm::Value*> caseComparisons;
      for (auto* caseValElem : section.first) {
        if (caseValElem->getType() == MatchType::mixOrChoice) {
          auto* cMatch = caseValElem->asMixOrChoice();
          if (cMatch->hasValueName()) {
            ctx->Error("Destructuring an expression of choice type by name is not supported",
                       cMatch->getValueName().range);
          }
          for (const auto& mField : mentionedFields) {
            if (mField.value == cMatch->get_name().value) {
              ctx->Error("The variant " + ctx->color(cMatch->get_name().value) + " of choice type " +
                             ctx->color(chTy->get_full_name()) +
                             " is repeating here. Please check logic and make necessary changes",
                         cMatch->getMainRange(),
                         Pair<String, FileRange>{"The previous occurrence of " + ctx->color(cMatch->get_name().value) +
                                                     " can be found here",
                                                 cMatch->getMainRange()});
            }
          }
          mentionedFields.push_back(cMatch->get_name());
          if (chTy->has_field(cMatch->get_name().value)) {
            caseComparisons.push_back(
                ctx->irCtx->builder.CreateICmpEQ(choiceVal, chTy->get_value_for(cMatch->get_name().value)));
          } else {
            ctx->Error("No variant named " + ctx->color(cMatch->get_name().value) + " in choice type " +
                           ctx->color(chTy->get_full_name()),
                       cMatch->get_name().range);
          }
        } else if (caseValElem->getType() == MatchType::Exp) {
          auto* eMatch  = caseValElem->asExp();
          auto* caseExp = eMatch->getExpression()->emit(ctx);
          if (caseExp->get_ir_type()->is_choice() ||
              (caseExp->get_ir_type()->is_reference() &&
               caseExp->get_ir_type()->as_reference()->get_subtype()->is_choice())) {
            if (caseExp->get_ir_type()->is_choice() && caseExp->is_ghost_pointer()) {
              caseComparisons.push_back(ctx->irCtx->builder.CreateICmpEQ(
                  choiceVal, ctx->irCtx->builder.CreateLoad(chTy->get_llvm_type(), caseExp->get_llvm())));
            } else if (caseExp->get_ir_type()->is_reference()) {
              caseExp->load_ghost_pointer(ctx->irCtx->builder);
              caseComparisons.push_back(ctx->irCtx->builder.CreateICmpEQ(
                  choiceVal, ctx->irCtx->builder.CreateLoad(chTy->get_llvm_type(), caseExp->get_llvm())));
            }
          } else {
            ctx->Error("Expected either the name of a variant of, "
                       "or an expression of type " +
                           ctx->color(chTy->get_full_name()),
                       eMatch->getMainRange());
          }
        } else {
          ctx->Error("Unexpected kind of match value found here, it should either be a choice match or an expression",
                     caseValElem->getMainRange());
        }
      }
      auto* trueBlock = new ir::Block(ctx->get_fn(), curr);
      // NOTE - Maybe change this?
      falseBlock = nullptr;
      llvm::Value* cond;
      if (caseComparisons.size() > 1) {
        cond = ctx->irCtx->builder.CreateOr(caseComparisons);
      } else {
        cond = caseComparisons.front();
      }
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        falseBlock = new ir::Block(ctx->get_fn(), curr);
        ctx->irCtx->builder.CreateCondBr(cond, trueBlock->get_bb(), falseBlock->get_bb());
      } else {
        ctx->irCtx->builder.CreateCondBr(cond, trueBlock->get_bb(), restBlock->get_bb());
      }
      trueBlock->set_active(ctx->irCtx->builder);
      emit_sentences(section.second, ctx);
      trueBlock->destroy_locals(ctx);
      (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        falseBlock->set_active(ctx->irCtx->builder);
      }
    }
    Vec<Identifier> missingFields;
    chTy->get_missing_names(mentionedFields, missingFields);
    if (missingFields.empty()) {
      if (elseCase.has_value()) {
        ctx->irCtx->Warning(
            "All possible variants of the choice type are already mentioned in the match sentence. No need to use else case for the match statement",
            elseCase.value().second);
      }
      elseNotRequired = true;
      if (falseBlock) {
        falseBlock->set_active(ctx->irCtx->builder);
        (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
      }
    } else if (!elseCase.has_value()) {
      ctx->Error(
          "Not all possible variants of the choice type are provided. Please add the else case to handle all missing variants",
          fileRange);
    }
  } else if (expTy->is_string_slice()) {
    auto*        strTy = expTy->as_string_slice();
    llvm::Value* strBuff;
    llvm::Value* strCount;
    bool         isMatchStrConstant = false;
    if (expEmit->is_prerun_value()) {
      isMatchStrConstant = true;
      strBuff            = expEmit->as_prerun()->get_llvm_constant()->getAggregateElement(0u);
      strCount           = expEmit->as_prerun()->get_llvm_constant()->getAggregateElement(1u);
    } else {
      if (expEmit->is_ghost_pointer() || expEmit->is_reference()) {
        if (expEmit->is_reference()) {
          expEmit->load_ghost_pointer(ctx->irCtx->builder);
        }
      }
      strBuff  = expEmit->is_value()
                     ? ctx->irCtx->builder.CreateExtractValue(expEmit->get_llvm(), {0u})
                     : ctx->irCtx->builder.CreateLoad(
                          llvm::Type::getInt8PtrTy(ctx->irCtx->llctx),
                          ctx->irCtx->builder.CreateStructGEP(strTy->get_llvm_type(), expEmit->get_llvm(), 0u));
      strCount = expEmit->is_value()
                     ? ctx->irCtx->builder.CreateExtractValue(expEmit->get_llvm(), {1u})
                     : ctx->irCtx->builder.CreateLoad(
                           llvm::Type::getInt64Ty(ctx->irCtx->llctx),
                           ctx->irCtx->builder.CreateStructGEP(strTy->get_llvm_type(), expEmit->get_llvm(), 1u));
    }
    for (auto& section : chain) {
      SHOW("Number of match values for this case: " << section.first.size())
      Vec<ir::Value*> irStrVals;
      bool            hasConstantVals   = false;
      bool            areAllConstValues = true;
      for (auto* strExp : section.first) {
        if (strExp->getType() != MatchType::Exp) {
          ctx->Error(
              "Invalid match type. Expected an expression in this match case since the candidate expression to match is a string slice",
              strExp->getMainRange());
        }
        auto* strEmit = strExp->asExp()->getExpression()->emit(ctx);
        irStrVals.push_back(strEmit);
        if (strEmit->is_prerun_value()) {
          hasConstantVals = true;
        } else {
          areAllConstValues = false;
        }
      }
      if (hasConstantVals && isMatchStrConstant) {
        bool caseRes = false;
        for (auto* irStr : irStrVals) {
          if (irStr->is_prerun_value()) {
            auto* irStrConst = irStr->get_llvm_constant();
            SHOW("Comparing constant string in match block")
            if (ir::Logic::compareConstantStrings(
                    llvm::cast<llvm::Constant>(strBuff), llvm::cast<llvm::Constant>(strCount),
                    irStrConst->getAggregateElement(0u), irStrConst->getAggregateElement(1u), ctx->irCtx->llctx)) {
              caseRes = true;
              break;
            }
          }
        }
        matchResult.push_back(CaseResult(caseRes, areAllConstValues));
        SHOW("Case constant result: " << (caseRes ? "true" : "false"))
        if (caseRes) {
          auto* trueBlock = new ir::Block(ctx->get_fn(), curr);
          (void)ir::add_branch(ctx->irCtx->builder, trueBlock->get_bb());
          trueBlock->set_active(ctx->irCtx->builder);
          emit_sentences(section.second, ctx);
          trueBlock->destroy_locals(ctx);
          (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
          break;
        } else {
          if (areAllConstValues) {
            continue;
          }
        }
      } else {
        matchResult.push_back(CaseResult(None, false));
      }
      SHOW("Creating case true block")
      auto* caseTrueBlock = new ir::Block(ctx->get_fn(), curr);
      SHOW("Creating case false block")
      auto* checkFalseBlock = new ir::Block(ctx->get_fn(), curr);
      SHOW("Setting thisCaseFalseBlock")
      ir::Block* thisCaseFalseBlock;
      for (usize j = 0; j < section.first.size(); j++) {
        if (irStrVals.at(j)->is_prerun_value() && isMatchStrConstant) {
          continue;
        }
        if (j == section.first.size() - 1) {
          thisCaseFalseBlock = checkFalseBlock;
        } else {
          thisCaseFalseBlock = new ir::Block(ctx->get_fn(), curr);
        }
        ir::Value*   caseIR       = irStrVals.at(j);
        llvm::Value* caseStrBuff  = nullptr;
        llvm::Value* caseStrCount = nullptr;
        // FIXME - Add optimisation for constant strings
        if (caseIR->get_ir_type()->is_string_slice() ||
            (caseIR->is_reference() && caseIR->get_ir_type()->as_reference()->get_subtype()->is_string_slice())) {
          auto* elemIter = ctx->get_fn()->get_str_comparison_index();
          if (caseIR->is_prerun_value()) {
            caseStrBuff  = caseIR->get_llvm_constant()->getAggregateElement(0u);
            caseStrCount = caseIR->get_llvm_constant()->getAggregateElement(1u);
          } else {
            if (caseIR->is_reference()) {
              caseIR->load_ghost_pointer(ctx->irCtx->builder);
            }
            caseStrBuff  = caseIR->is_value()
                               ? ctx->irCtx->builder.CreateExtractValue(caseIR->get_llvm(), {0u})
                               : ctx->irCtx->builder.CreateLoad(llvm::Type::getInt8PtrTy(ctx->irCtx->llctx),
                                                                ctx->irCtx->builder.CreateStructGEP(
                                                                   strTy->get_llvm_type(), caseIR->get_llvm(), 0u));
            caseStrCount = ctx->irCtx->builder.CreateLoad(
                llvm::Type::getInt64Ty(ctx->irCtx->llctx),
                caseIR->is_value()
                    ? ctx->irCtx->builder.CreateExtractValue(caseIR->get_llvm(), {1u})
                    : ctx->irCtx->builder.CreateStructGEP(strTy->get_llvm_type(), caseIR->get_llvm(), 1u));
          }
          auto* Ty64Int = llvm::Type::getInt64Ty(ctx->irCtx->llctx);
          SHOW("Creating lenCheckTrueBlock")
          auto* lenCheckTrueBlock = new ir::Block(ctx->get_fn(), curr);
          ctx->irCtx->builder.CreateCondBr(ctx->irCtx->builder.CreateICmpEQ(strCount, caseStrCount),
                                           lenCheckTrueBlock->get_bb(), thisCaseFalseBlock->get_bb());
          lenCheckTrueBlock->set_active(ctx->irCtx->builder);
          ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(Ty64Int, 0u), elemIter->get_llvm());
          SHOW("Creating iter cond block")
          auto* iterCondBlock = new ir::Block(ctx->get_fn(), lenCheckTrueBlock);
          SHOW("Creating iter true block")
          auto* iterTrueBlock = new ir::Block(ctx->get_fn(), lenCheckTrueBlock);
          SHOW("Creating iter false block")
          auto* iterFalseBlock = new ir::Block(ctx->get_fn(), lenCheckTrueBlock);
          (void)ir::add_branch(ctx->irCtx->builder, iterCondBlock->get_bb());
          iterCondBlock->set_active(ctx->irCtx->builder);
          ctx->irCtx->builder.CreateCondBr(
              ctx->irCtx->builder.CreateICmpULT(ctx->irCtx->builder.CreateLoad(Ty64Int, elemIter->get_llvm()),
                                                caseStrCount),
              iterTrueBlock->get_bb(), iterFalseBlock->get_bb());
          iterTrueBlock->set_active(ctx->irCtx->builder);
          auto* Ty8Int = llvm::Type::getInt8Ty(ctx->irCtx->llctx);
          SHOW("Creating iter incr block")
          auto* iterIncrBlock = new ir::Block(ctx->get_fn(), lenCheckTrueBlock);
          ctx->irCtx->builder.CreateCondBr(
              ctx->irCtx->builder.CreateICmpEQ(
                  ctx->irCtx->builder.CreateLoad(
                      Ty8Int, ctx->irCtx->builder.CreateInBoundsGEP(
                                  Ty8Int, strBuff, {ctx->irCtx->builder.CreateLoad(Ty64Int, elemIter->get_llvm())})),
                  ctx->irCtx->builder.CreateLoad(
                      Ty8Int,
                      ctx->irCtx->builder.CreateInBoundsGEP(
                          Ty8Int, caseStrBuff, {ctx->irCtx->builder.CreateLoad(Ty64Int, elemIter->get_llvm())}))),
              iterIncrBlock->get_bb(), iterFalseBlock->get_bb());
          iterIncrBlock->set_active(ctx->irCtx->builder);
          ctx->irCtx->builder.CreateStore(
              ctx->irCtx->builder.CreateAdd(ctx->irCtx->builder.CreateLoad(Ty64Int, elemIter->get_llvm()),
                                            llvm::ConstantInt::get(Ty64Int, 1u)),
              elemIter->get_llvm());
          (void)ir::add_branch(ctx->irCtx->builder, iterCondBlock->get_bb());
          iterFalseBlock->set_active(ctx->irCtx->builder);
          ctx->irCtx->builder.CreateCondBr(
              ctx->irCtx->builder.CreateICmpEQ(strCount, ctx->irCtx->builder.CreateLoad(Ty64Int, elemIter->get_llvm())),
              caseTrueBlock->get_bb(), thisCaseFalseBlock->get_bb());
        } else {
          ctx->Error("Expected a string slice to be the expression for this match case",
                     section.first.at(j)->getMainRange());
        }
        SHOW("Setting thisCaseFalseBlock active")
        thisCaseFalseBlock->set_active(ctx->irCtx->builder);
      }
      caseTrueBlock->set_active(ctx->irCtx->builder);
      emit_sentences(section.second, ctx);
      caseTrueBlock->destroy_locals(ctx);
      (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
      checkFalseBlock->set_active(ctx->irCtx->builder);
    }
  }
  if (elseCase.has_value()) {
    // FIXME - Fix condition when match blocks can return a value
    if (elseNotRequired) {
      ctx->irCtx->Warning(
          ctx->irCtx->highlightWarning("else") +
              " case is not required in this match block. Sentences inside the else block will not be emitted/compiled",
          elseCase.value().second);
    } else {
      if (isFalseForAllCases() && hasConstResultForAllCases()) {
        auto* elseBlock = new ir::Block(ctx->get_fn(), curr);
        (void)ir::add_branch(ctx->irCtx->builder, elseBlock->get_bb());
        elseBlock->set_active(ctx->irCtx->builder);
        emit_sentences(elseCase.value().first, ctx);
        elseBlock->destroy_locals(ctx);
        (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
      } else if (!isTrueForACase()) {
        auto* activeBlock = ctx->get_fn()->get_block();
        emit_sentences(elseCase.value().first, ctx);
        activeBlock->destroy_locals(ctx);
        (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
      }
    }
  } else {
    if (!elseNotRequired && !isTrueForACase()) {
      ctx->Error(
          "A string slice has infinite number of patterns to match. Please add an else case to account for the remaining possibilies",
          fileRange);
    }
  }
  restBlock->set_active(ctx->irCtx->builder);
  // TODO - Add regexp support
  return nullptr;
}

bool Match::hasConstResultForAllCases() {
  for (auto& val : matchResult) {
    if (!val.result.has_value() || !val.areAllConstant) {
      return false;
    }
  }
  return true;
}

bool Match::isFalseForAllCases() {
  for (auto& val : matchResult) {
    if (val.result.has_value()) {
      if (val.result.value()) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

bool Match::isTrueForACase() {
  for (auto& val : matchResult) {
    if (val.result.has_value()) {
      if (val.result.value()) {
        return true;
      }
    }
  }
  return false;
}

Json Match::to_json() const {
  Vec<JsonValue> chainJson;
  for (const auto& elem : chain) {
    Vec<JsonValue> sentencesJson;
    SHOW("Converting sentences to JSON")
    for (auto* snt : elem.second) {
      SHOW("Sentence node type is: ")
      SHOW((int)snt->nodeType())
      SHOW("Got node type")
      sentencesJson.push_back(snt->to_json());
    }
    Vec<JsonValue> matchValsJson;
    for (auto* mVal : elem.first) {
      SHOW("Match value type is: " << (int)mVal->getType())
      matchValsJson.push_back(mVal->to_json());
    }
    chainJson.push_back(Json()._("matchValues", matchValsJson)._("sentences", sentencesJson));
  }
  Vec<JsonValue> elseJson;
  if (elseCase.has_value()) {
    for (auto* snt : elseCase.value().first) {
      elseJson.push_back(snt->to_json());
    }
  }
  return Json()
      ._("candidate", candidate->to_json())
      ._("matchChain", chainJson)
      ._("hasElse", elseCase.has_value())
      ._("elseSentences", elseJson)
      ._("elseRange", elseCase.has_value() ? (Json)(elseCase->second) : Json());
}

} // namespace qat::ast
