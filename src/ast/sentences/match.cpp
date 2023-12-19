#include "./match.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

MixOrChoiceMatchValue* MatchValue::asMixOrChoice() { return (MixOrChoiceMatchValue*)this; }
ExpressionMatchValue*  MatchValue::asExp() { return (ExpressionMatchValue*)this; }

MixOrChoiceMatchValue::MixOrChoiceMatchValue(Identifier _name, Maybe<Identifier> _valueName, bool _isVar)
    : name(std::move(_name)), valueName(std::move(_valueName)), isVar(_isVar) {}

Identifier MixOrChoiceMatchValue::getName() const { return name; }

bool MixOrChoiceMatchValue::hasValueName() const { return valueName.has_value(); }

bool MixOrChoiceMatchValue::isVariable() const { return isVar; }

Identifier MixOrChoiceMatchValue::getValueName() const { return valueName.value(); }

Json MixOrChoiceMatchValue::toJson() const {
  return Json()
      ._("matchValueType", "mix")
      ._("name", name)
      ._("hasValue", valueName.has_value())
      ._("valueName", valueName.has_value() ? valueName.value() : JsonValue());
}

ExpressionMatchValue::ExpressionMatchValue(Expression* _exp) : exp(_exp) {}

Expression* ExpressionMatchValue::getExpression() const { return exp; }

Json ExpressionMatchValue::toJson() const {
  SHOW("Expression node type is: " << (int)exp->nodeType())
  return Json()._("matchValueType", "expression")._("expression", exp->toJson());
}

CaseResult::CaseResult(Maybe<bool> _result, bool _areAllConst) : result(_result), areAllConstant(_areAllConst) {}

Match::Match(bool _isTypeMatch, Expression* _candidate, Vec<Pair<Vec<MatchValue*>, Vec<Sentence*>>> _chain,
             Maybe<Pair<Vec<Sentence*>, FileRange>> _elseCase, FileRange _fileRange)
    : Sentence(std::move(_fileRange)), isTypeMatch(_isTypeMatch), candidate(_candidate), chain(std::move(_chain)),
      elseCase(std::move(_elseCase)) {}

IR::Value* Match::emit(IR::Context* ctx) {
  auto* expEmit       = candidate->emit(ctx);
  auto* expTy         = expEmit->getType();
  bool  isExpVariable = false;
  if (expTy->isReference()) {
    expEmit->loadImplicitPointer(ctx->builder);
    isExpVariable = expTy->asReference()->isSubtypeVariable();
    expTy         = expTy->asReference()->getSubType();
  } else {
    isExpVariable = expEmit->isVariable();
  }
  auto* curr      = ctx->getActiveFunction()->getBlock();
  auto* restBlock = new IR::Block(ctx->getActiveFunction(), nullptr);
  restBlock->linkPrevBlock(curr);
  bool elseNotRequired = false;
  if (expTy->isMix()) {
    auto* mTy = expTy->asMix();
    if (!expEmit->isReference() && !expEmit->isImplicitPointer()) {
      expEmit->makeImplicitPointer(ctx, None);
    }
    Vec<Identifier> mentionedFields;
    IR::Block*      falseBlock = nullptr;
    for (usize i = 0; i < chain.size(); i++) {
      const auto&       section = chain.at(i);
      Vec<llvm::Value*> conditions;
      for (usize k = 0; k < section.first.size(); k++) {
        if (section.first.at(k)->getType() != MatchType::mixOrChoice) {
          ctx->Error("Expected the name of a variant of the mix type " +
                         ctx->highlightError(expTy->asMix()->getFullName()),
                     section.first.at(k)->getMainRange());
        }
        auto* uMatch = section.first.at(k)->asMixOrChoice();
        if (uMatch->isVariable()) {
          if (!isExpVariable) {
            ctx->Error("The expression being matched does not possess variability and "
                       "hence the matched "
                       "case cannot use a variable reference to the subtype value",
                       uMatch->getValueName().range);
          }
        }
        auto subRes = mTy->hasSubTypeWithName(uMatch->getName().value);
        if (!subRes.first) {
          ctx->Error("No field named " + ctx->highlightError(uMatch->getName().value) + " in mix type " +
                         ctx->highlightError(mTy->getFullName()),
                     uMatch->getName().range);
        }
        if (uMatch->hasValueName()) {
          if (section.first.size() > 1) {
            ctx->Error(
                "Multiple values are provided to be matched for this case, and hence the matched value cannot be requested for use",
                uMatch->getMainRange());
          }
        }
        if (!subRes.second && uMatch->hasValueName()) {
          ctx->Error("Sub-field " + ctx->highlightError(uMatch->getName().value) + " of mix type " +
                         ctx->highlightError(mTy->getFullName()) +
                         " does not have a type associated with it, and hence you "
                         "cannot use a name for the value here",
                     uMatch->getValueName().range);
        }
        // NOTE - When expressions are supported for mix types, take care of this
        for (usize mixValIdx = k + 1; mixValIdx < section.first.size(); mixValIdx++) {
          if (uMatch->getName().value == chain.at(i).first.at(mixValIdx)->asMixOrChoice()->getName().value) {
            ctx->Error("Repeating subfield of the mix type ",
                       chain.at(i).first.at(mixValIdx)->asMixOrChoice()->getName().range);
          }
        }
        for (usize j = i + 1; j < chain.size(); j++) {
          for (auto* matchVal : chain.at(j).first) {
            if (uMatch->getName().value == matchVal->asMixOrChoice()->getName().value) {
              ctx->Error("Repeating subfield of the mix type " + ctx->highlightError(mTy->getFullName()),
                         matchVal->asMixOrChoice()->getName().range);
            }
          }
        }
        mentionedFields.push_back(uMatch->getName());
        conditions.push_back(ctx->builder.CreateICmpEQ(
            ctx->builder.CreateLoad(llvm::Type::getIntNTy(ctx->llctx, mTy->getTagBitwidth()),
                                    ctx->builder.CreateStructGEP(mTy->getLLVMType(), expEmit->getLLVM(), 0)),
            llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, mTy->getTagBitwidth()),
                                   mTy->getIndexOfName(uMatch->getName().value))));
      }
      auto* trueBlock = new IR::Block(ctx->getActiveFunction(), curr);
      falseBlock      = nullptr;
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        falseBlock = new IR::Block(ctx->getActiveFunction(), curr);
        if (conditions.size() == 1) {
          ctx->builder.CreateCondBr(conditions.front(), trueBlock->getBB(), falseBlock->getBB());
        } else {
          ctx->builder.CreateCondBr(ctx->builder.CreateOr(conditions), trueBlock->getBB(), falseBlock->getBB());
        }
      } else {
        if (conditions.size() == 1) {
          ctx->builder.CreateCondBr(conditions.front(), trueBlock->getBB(), restBlock->getBB());
        } else {
          ctx->builder.CreateCondBr(ctx->builder.CreateOr(conditions), trueBlock->getBB(), restBlock->getBB());
        }
      }
      trueBlock->setActive(ctx->builder);
      SHOW("True block name is: " << trueBlock->getName())
      if ((section.first.size() == 1) && section.first.front()->asMixOrChoice()->hasValueName()) {
        auto* uMatch = section.first.front()->asMixOrChoice();
        SHOW("Match case has value name: " << uMatch->getValueName().value)
        if (trueBlock->hasValue(uMatch->getValueName().value)) {
          SHOW("Local entity with match case value name exists")
          ctx->Error("Local entity named " + ctx->highlightError(uMatch->getValueName().value) +
                         " exists already in this scope",
                     uMatch->getValueName().range);
        } else {
          SHOW("Creating local entity for match case value named: " << uMatch->getValueName().value)
          auto* loc = trueBlock->newValue(
              uMatch->getValueName().value,
              IR::ReferenceType::get(uMatch->isVariable(), mTy->getSubTypeWithName(uMatch->getName().value), ctx),
              false, uMatch->getValueName().range);
          SHOW("Local Entity for match case created")
          ctx->builder.CreateStore(ctx->builder.CreatePointerCast(
                                       ctx->builder.CreateStructGEP(mTy->getLLVMType(), expEmit->getLLVM(), 1),
                                       mTy->getSubTypeWithName(uMatch->getName().value)->getLLVMType()->getPointerTo()),
                                   loc->getAlloca());
        }
      }
      emitSentences(section.second, ctx);
      trueBlock->destroyLocals(ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        falseBlock->setActive(ctx->builder);
      }
    }
    Vec<Identifier> missingFields;
    mTy->getMissingNames(mentionedFields, missingFields);
    if (missingFields.empty()) {
      if (elseCase.has_value()) {
        ctx->Warning("All possible variants of the mix type are already "
                     "mentioned in the match sentence. No need to use else case "
                     "for the match statement",
                     elseCase.value().second);
      }
      elseNotRequired = true;
      if (falseBlock) {
        falseBlock->setActive(ctx->builder);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
      }
    } else {
      if (!elseCase.has_value()) {
        ctx->Error("Not all possible variants of the mix type are provided. "
                   "Please add the else case to handle all missing variants",
                   fileRange);
      }
    }
  } else if (expTy->isChoice()) {
    auto*        chTy = expTy->asChoice();
    llvm::Value* choiceVal;
    if (expEmit->isReference() || expEmit->isImplicitPointer()) {
      choiceVal = ctx->builder.CreateLoad(chTy->getLLVMType(), expEmit->getLLVM());
    } else {
      choiceVal = expEmit->getLLVM();
    }
    Vec<Identifier> mentionedFields;
    IR::Block*      falseBlock = nullptr;
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
            if (mField.value == cMatch->getName().value) {
              ctx->Error("The variant " + ctx->highlightError(cMatch->getName().value) + " of choice type " +
                             ctx->highlightError(chTy->getFullName()) +
                             " is repeating here. Please check logic and make necessary changes",
                         cMatch->getMainRange());
            }
          }
          mentionedFields.push_back(cMatch->getName());
          if (chTy->hasField(cMatch->getName().value)) {
            caseComparisons.push_back(ctx->builder.CreateICmpEQ(choiceVal, chTy->getValueFor(cMatch->getName().value)));
          } else {
            ctx->Error("No variant named " + ctx->highlightError(cMatch->getName().value) + " in choice type " +
                           ctx->highlightError(chTy->getFullName()),
                       cMatch->getName().range);
          }
          ctx->Error("Expected the name of a variant of the choice type " +
                         ctx->highlightError(expTy->asMix()->getFullName()),
                     caseValElem->getMainRange());
        } else if (caseValElem->getType() == MatchType::Exp) {
          auto* eMatch  = caseValElem->asExp();
          auto* caseExp = eMatch->getExpression()->emit(ctx);
          if (caseExp->getType()->isChoice() ||
              (caseExp->getType()->isReference() && caseExp->getType()->asReference()->getSubType()->isChoice())) {
            if (caseExp->getType()->isChoice() && caseExp->isImplicitPointer()) {
              caseComparisons.push_back(ctx->builder.CreateICmpEQ(
                  choiceVal, ctx->builder.CreateLoad(chTy->getLLVMType(), caseExp->getLLVM())));
            } else if (caseExp->getType()->isReference()) {
              caseExp->loadImplicitPointer(ctx->builder);
              caseComparisons.push_back(ctx->builder.CreateICmpEQ(
                  choiceVal, ctx->builder.CreateLoad(chTy->getLLVMType(), caseExp->getLLVM())));
            }
          } else {
            ctx->Error("Expected either the name of a variant of, "
                       "or an expression of type " +
                           ctx->highlightError(chTy->getFullName()),
                       eMatch->getMainRange());
          }
        } else {
          ctx->Error("Unexpected match value type here", caseValElem->getMainRange());
        }
      }
      auto* trueBlock = new IR::Block(ctx->getActiveFunction(), curr);
      // NOTE - Maybe change this?
      falseBlock = nullptr;
      llvm::Value* cond;
      if (caseComparisons.size() > 1) {
        cond = ctx->builder.CreateOr(caseComparisons);
      } else {
        cond = caseComparisons.front();
      }
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        falseBlock = new IR::Block(ctx->getActiveFunction(), curr);
        ctx->builder.CreateCondBr(cond, trueBlock->getBB(), falseBlock->getBB());
      } else {
        ctx->builder.CreateCondBr(cond, trueBlock->getBB(), restBlock->getBB());
      }
      trueBlock->setActive(ctx->builder);
      emitSentences(section.second, ctx);
      trueBlock->destroyLocals(ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        falseBlock->setActive(ctx->builder);
      }
    }
    Vec<Identifier> missingFields;
    chTy->getMissingNames(mentionedFields, missingFields);
    if (missingFields.empty()) {
      if (elseCase.has_value()) {
        ctx->Warning(
            "All possible variants of the choice type are already mentioned in the match sentence. No need to use else case for the match statement",
            elseCase.value().second);
      }
      elseNotRequired = true;
      if (falseBlock) {
        falseBlock->setActive(ctx->builder);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
      }
    } else if (!elseCase.has_value()) {
      ctx->Error(
          "Not all possible variants of the choice type are provided. Please add the else case to handle all missing variants",
          fileRange);
    }
  } else if (expTy->isStringSlice()) {
    auto*        strTy = expTy->asStringSlice();
    llvm::Value* strBuff;
    llvm::Value* strCount;
    bool         isMatchStrConstant = false;
    if (expEmit->isPrerunValue()) {
      isMatchStrConstant = true;
      strBuff            = expEmit->asConst()->getLLVMConstant()->getAggregateElement(0u);
      strCount           = expEmit->asConst()->getLLVMConstant()->getAggregateElement(1u);
    } else {
      if (expEmit->isImplicitPointer() || expEmit->isReference()) {
        if (expEmit->isReference()) {
          expEmit->loadImplicitPointer(ctx->builder);
        }
      } else {
        expEmit->makeImplicitPointer(ctx, None);
      }
      strBuff  = ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(ctx->llctx),
                                         ctx->builder.CreateStructGEP(strTy->getLLVMType(), expEmit->getLLVM(), 0u));
      strCount = ctx->builder.CreateLoad(llvm::Type::getInt64Ty(ctx->llctx),
                                         ctx->builder.CreateStructGEP(strTy->getLLVMType(), expEmit->getLLVM(), 1u));
    }
    for (auto& section : chain) {
      SHOW("Number of match values for this case: " << section.first.size())
      Vec<IR::Value*> irStrVals;
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
        if (strEmit->isPrerunValue()) {
          hasConstantVals = true;
        } else {
          areAllConstValues = false;
        }
      }
      if (hasConstantVals && isMatchStrConstant) {
        bool caseRes = false;
        for (auto* irStr : irStrVals) {
          if (irStr->isPrerunValue()) {
            auto* irStrConst = irStr->getLLVMConstant();
            SHOW("Comparing constant string in match block")
            if (IR::Logic::compareConstantStrings(
                    llvm::cast<llvm::Constant>(strBuff), llvm::cast<llvm::Constant>(strCount),
                    irStrConst->getAggregateElement(0u), irStrConst->getAggregateElement(1u), ctx->llctx)) {
              caseRes = true;
              break;
            }
          }
        }
        matchResult.push_back(CaseResult(caseRes, areAllConstValues));
        SHOW("Case constant result: " << (caseRes ? "true" : "false"))
        if (caseRes) {
          auto* trueBlock = new IR::Block(ctx->getActiveFunction(), curr);
          (void)IR::addBranch(ctx->builder, trueBlock->getBB());
          trueBlock->setActive(ctx->builder);
          emitSentences(section.second, ctx);
          trueBlock->destroyLocals(ctx);
          (void)IR::addBranch(ctx->builder, restBlock->getBB());
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
      auto* caseTrueBlock = new IR::Block(ctx->getActiveFunction(), curr);
      SHOW("Creating case false block")
      auto* checkFalseBlock = new IR::Block(ctx->getActiveFunction(), curr);
      SHOW("Setting thisCaseFalseBlock")
      IR::Block* thisCaseFalseBlock;
      for (usize j = 0; j < section.first.size(); j++) {
        if (irStrVals.at(j)->isPrerunValue() && isMatchStrConstant) {
          continue;
        }
        if (j == section.first.size() - 1) {
          thisCaseFalseBlock = checkFalseBlock;
        } else {
          thisCaseFalseBlock = new IR::Block(ctx->getActiveFunction(), curr);
        }
        IR::Value*   caseIR       = irStrVals.at(j);
        llvm::Value* caseStrBuff  = nullptr;
        llvm::Value* caseStrCount = nullptr;
        // FIXME - Add optimisation for constant strings
        if (caseIR->getType()->isStringSlice() ||
            (caseIR->isReference() && caseIR->getType()->asReference()->getSubType()->isStringSlice())) {
          auto* elemIter = ctx->getActiveFunction()->getFunctionCommonIndex();
          if (caseIR->isPrerunValue()) {
            caseStrBuff  = caseIR->getLLVMConstant()->getAggregateElement(0u);
            caseStrCount = caseIR->getLLVMConstant()->getAggregateElement(1u);
          } else {
            if (caseIR->isReference()) {
              caseIR->loadImplicitPointer(ctx->builder);
            } else if (!caseIR->isImplicitPointer()) {
              caseIR->makeImplicitPointer(ctx, None);
            }
            caseStrBuff =
                ctx->builder.CreateLoad(llvm::Type::getInt8PtrTy(ctx->llctx),
                                        ctx->builder.CreateStructGEP(strTy->getLLVMType(), caseIR->getLLVM(), 0u));
            caseStrCount =
                ctx->builder.CreateLoad(llvm::Type::getInt64Ty(ctx->llctx),
                                        ctx->builder.CreateStructGEP(strTy->getLLVMType(), caseIR->getLLVM(), 1u));
          }
          auto* Ty64Int = llvm::Type::getInt64Ty(ctx->llctx);
          SHOW("Creating lenCheckTrueBlock")
          auto* lenCheckTrueBlock = new IR::Block(ctx->getActiveFunction(), curr);
          ctx->builder.CreateCondBr(ctx->builder.CreateICmpEQ(strCount, caseStrCount), lenCheckTrueBlock->getBB(),
                                    thisCaseFalseBlock->getBB());
          lenCheckTrueBlock->setActive(ctx->builder);
          ctx->builder.CreateStore(llvm::ConstantInt::get(Ty64Int, 0u), elemIter->getLLVM());
          SHOW("Creating iter cond block")
          auto* iterCondBlock = new IR::Block(ctx->getActiveFunction(), lenCheckTrueBlock);
          SHOW("Creating iter true block")
          auto* iterTrueBlock = new IR::Block(ctx->getActiveFunction(), lenCheckTrueBlock);
          SHOW("Creating iter false block")
          auto* iterFalseBlock = new IR::Block(ctx->getActiveFunction(), lenCheckTrueBlock);
          (void)IR::addBranch(ctx->builder, iterCondBlock->getBB());
          iterCondBlock->setActive(ctx->builder);
          ctx->builder.CreateCondBr(
              ctx->builder.CreateICmpULT(ctx->builder.CreateLoad(Ty64Int, elemIter->getLLVM()), caseStrCount),
              iterTrueBlock->getBB(), iterFalseBlock->getBB());
          iterTrueBlock->setActive(ctx->builder);
          auto* Ty8Int = llvm::Type::getInt8Ty(ctx->llctx);
          SHOW("Creating iter incr block")
          auto* iterIncrBlock = new IR::Block(ctx->getActiveFunction(), lenCheckTrueBlock);
          ctx->builder.CreateCondBr(
              ctx->builder.CreateICmpEQ(
                  ctx->builder.CreateLoad(
                      Ty8Int, ctx->builder.CreateInBoundsGEP(Ty8Int, strBuff,
                                                             {ctx->builder.CreateLoad(Ty64Int, elemIter->getLLVM())})),
                  ctx->builder.CreateLoad(
                      Ty8Int, ctx->builder.CreateInBoundsGEP(Ty8Int, caseStrBuff,
                                                             {ctx->builder.CreateLoad(Ty64Int, elemIter->getLLVM())}))),
              iterIncrBlock->getBB(), iterFalseBlock->getBB());
          iterIncrBlock->setActive(ctx->builder);
          ctx->builder.CreateStore(ctx->builder.CreateAdd(ctx->builder.CreateLoad(Ty64Int, elemIter->getLLVM()),
                                                          llvm::ConstantInt::get(Ty64Int, 1u)),
                                   elemIter->getLLVM());
          (void)IR::addBranch(ctx->builder, iterCondBlock->getBB());
          iterFalseBlock->setActive(ctx->builder);
          ctx->builder.CreateCondBr(
              ctx->builder.CreateICmpEQ(strCount, ctx->builder.CreateLoad(Ty64Int, elemIter->getLLVM())),
              caseTrueBlock->getBB(), thisCaseFalseBlock->getBB());
        } else {
          ctx->Error("Expected a string slice to be the expression for this match case",
                     section.first.at(j)->getMainRange());
        }
        SHOW("Setting thisCaseFalseBlock active")
        thisCaseFalseBlock->setActive(ctx->builder);
      }
      caseTrueBlock->setActive(ctx->builder);
      emitSentences(section.second, ctx);
      caseTrueBlock->destroyLocals(ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      checkFalseBlock->setActive(ctx->builder);
    }
  }
  if (elseCase.has_value()) {
    // FIXME - Fix condition when match blocks can return a value
    if (elseNotRequired) {
      ctx->Warning(
          ctx->highlightWarning("else") +
              " case is not required in this match block. Sentences inside the else block will not be emitted/compiled",
          elseCase.value().second);
    } else {
      if (isFalseForAllCases() && hasConstResultForAllCases()) {
        auto* elseBlock = new IR::Block(ctx->getActiveFunction(), curr);
        (void)IR::addBranch(ctx->builder, elseBlock->getBB());
        elseBlock->setActive(ctx->builder);
        emitSentences(elseCase.value().first, ctx);
        elseBlock->destroyLocals(ctx);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
      } else if (!isTrueForACase()) {
        auto* activeBlock = ctx->getActiveFunction()->getBlock();
        emitSentences(elseCase.value().first, ctx);
        activeBlock->destroyLocals(ctx);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
      }
    }
  } else {
    if (!elseNotRequired && !isTrueForACase()) {
      ctx->Error(
          "A string slice has infinite number of patterns to match. Please add an else case to account for the remaining possibilies",
          fileRange);
    }
  }
  restBlock->setActive(ctx->builder);
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

Json Match::toJson() const {
  Vec<JsonValue> chainJson;
  for (const auto& elem : chain) {
    Vec<JsonValue> sentencesJson;
    SHOW("Converting sentences to JSON")
    for (auto* snt : elem.second) {
      SHOW("Sentence node type is: ")
      SHOW((int)snt->nodeType())
      SHOW("Got node type")
      sentencesJson.push_back(snt->toJson());
    }
    Vec<JsonValue> matchValsJson;
    for (auto* mVal : elem.first) {
      SHOW("Match value type is: " << (int)mVal->getType())
      matchValsJson.push_back(mVal->toJson());
    }
    chainJson.push_back(Json()._("matchValues", matchValsJson)._("sentences", sentencesJson));
  }
  Vec<JsonValue> elseJson;
  if (elseCase.has_value()) {
    for (auto* snt : elseCase.value().first) {
      elseJson.push_back(snt->toJson());
    }
  }
  return Json()
      ._("candidate", candidate->toJson())
      ._("matchChain", chainJson)
      ._("hasElse", elseCase.has_value())
      ._("elseSentences", elseJson)
      ._("elseRange", elseCase.has_value() ? (Json)(elseCase->second) : Json());
}

} // namespace qat::ast