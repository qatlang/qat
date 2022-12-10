#include "./match.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

MixMatchValue*        MatchValue::asMix() { return (MixMatchValue*)this; }
ChoiceMatchValue*     MatchValue::asChoice() { return (ChoiceMatchValue*)this; }
ExpressionMatchValue* MatchValue::asExp() { return (ExpressionMatchValue*)this; }

MixMatchValue::MixMatchValue(Identifier _name, Maybe<Identifier> _valueName, bool _isVar)
    : name(std::move(_name)), valueName(std::move(_valueName)), isVar(_isVar) {}

Identifier MixMatchValue::getName() const { return name; }

bool MixMatchValue::hasValueName() const { return valueName.has_value(); }

bool MixMatchValue::isVariable() const { return isVar; }

Identifier MixMatchValue::getValueName() const { return valueName.value(); }

Json MixMatchValue::toJson() const {
  return Json()
      ._("matchValueType", "mix")
      ._("name", name)
      ._("hasValue", valueName.has_value())
      ._("valueName", valueName.has_value() ? valueName.value() : JsonValue());
}

ChoiceMatchValue::ChoiceMatchValue(Identifier _name) : name(std::move(_name)) {}

Identifier ChoiceMatchValue::getName() const { return name; }

FileRange ChoiceMatchValue::getFileRange() const { return name.range; }

Json ChoiceMatchValue::toJson() const { return Json()._("matchValueType", "choice")._("name", name); }

ExpressionMatchValue::ExpressionMatchValue(Expression* _exp) : exp(_exp) {}

Expression* ExpressionMatchValue::getExpression() const { return exp; }

Json ExpressionMatchValue::toJson() const {
  return Json()._("matchValueType", "expression")._("expression", exp->toJson());
}

Match::Match(bool _isTypeMatch, Expression* _candidate, Vec<Pair<MatchValue*, Vec<Sentence*>>> _chain,
             Maybe<Vec<Sentence*>> _elseCase, FileRange _fileRange)
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
  auto* curr      = ctx->fn->getBlock();
  auto* restBlock = new IR::Block(ctx->fn, nullptr);
  if (expTy->isMix()) {
    auto* mTy = expTy->asMix();
    if (!expEmit->isReference() && !expEmit->isImplicitPointer()) {
      ctx->Error("Invalid value for matching", candidate->fileRange);
    }
    Vec<Identifier> mentionedFields;
    for (usize i = 0; i < chain.size(); i++) {
      const auto& section = chain.at(i);
      if (section.first->getType() != MatchType::mix) {
        ctx->Error("Expected the name of a variant of the mix type " +
                       ctx->highlightError(expTy->asMix()->getFullName()),
                   section.first->getMainRange());
      }
      auto* uMatch = section.first->asMix();
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
      if (!subRes.second && uMatch->hasValueName()) {
        ctx->Error("Sub-field " + ctx->highlightError(uMatch->getName().value) + " of mix type " +
                       ctx->highlightError(mTy->getFullName()) +
                       " does not have a type associated with it, and hence you "
                       "cannot use a name for the value here",
                   uMatch->getValueName().range);
      }
      for (usize j = i + 1; j < chain.size(); j++) {
        if (uMatch->getName().value == chain.at(j).first->asMix()->getName().value) {
          ctx->Error("Repeating subfield of the mix type " + ctx->highlightError(mTy->getFullName()),
                     chain.at(j).first->asMix()->getName().range);
        }
      }
      mentionedFields.push_back(uMatch->getName());
      auto*      trueBlock  = new IR::Block(ctx->fn, curr);
      IR::Block* falseBlock = nullptr;
      auto*      cond       = ctx->builder.CreateICmpEQ(
          ctx->builder.CreateLoad(llvm::Type::getIntNTy(ctx->llctx, mTy->getTagBitwidth()),
                                             ctx->builder.CreateStructGEP(mTy->getLLVMType(), expEmit->getLLVM(), 0)),
          llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, mTy->getTagBitwidth()),
                                            mTy->getIndexOfName(uMatch->getName().value)));
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        falseBlock = new IR::Block(ctx->fn, curr);
        ctx->builder.CreateCondBr(cond, trueBlock->getBB(), falseBlock->getBB());
      } else {
        ctx->builder.CreateCondBr(cond, trueBlock->getBB(), restBlock->getBB());
      }
      trueBlock->setActive(ctx->builder);
      SHOW("True block name is: " << trueBlock->getName())
      if (section.first->asMix()->hasValueName()) {
        SHOW("Match case has value name: " << section.first->asMix()->getValueName().value)
        if (trueBlock->hasValue(uMatch->getValueName().value)) {
          SHOW("Local entity with match case value name exists")
          ctx->Error("Local entity named " + ctx->highlightError(section.first->asMix()->getValueName().value) +
                         " exists already in this scope",
                     section.first->asMix()->getValueName().range);
        } else {
          SHOW("Creating local entity for match case value named: " << uMatch->getValueName().value)
          auto* loc =
              trueBlock->newValue(uMatch->getValueName().value,
                                  IR::ReferenceType::get(uMatch->isVariable(),
                                                         mTy->getSubTypeWithName(uMatch->getName().value), ctx->llctx),
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
        ctx->Error("All possible variants of the mix type are already "
                   "mentioned in the match sentence. No need to use else case "
                   "for the match statement",
                   fileRange);
      }
    } else {
      if (!elseCase.has_value()) {
        ctx->Error("Not all possible variants of the mix type are provided. "
                   "Please add the else case to handle all missing variants",
                   fileRange);
      }
    }
    if (elseCase.has_value()) {
      emitSentences(elseCase.value(), ctx);
      ctx->fn->getBlock()->destroyLocals(ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
    }
    restBlock->setActive(ctx->builder);
  } else if (expTy->isChoice()) {
    auto*        chTy = expTy->asChoice();
    llvm::Value* choiceVal;
    if (expEmit->isReference() || expEmit->isImplicitPointer()) {
      choiceVal = ctx->builder.CreateLoad(chTy->getLLVMType(), expEmit->getLLVM());
    } else {
      choiceVal = expEmit->getLLVM();
    }
    Vec<Identifier> mentionedFields;
    for (usize i = 0; i < chain.size(); i++) {
      const auto&  section = chain.at(i);
      llvm::Value* caseVal;
      if (section.first->getType() == MatchType::mix) {
        ctx->Error("Expected the name of a variant of the choice type " +
                       ctx->highlightError(expTy->asMix()->getFullName()),
                   section.first->getMainRange());
      } else if (section.first->getType() == MatchType::Exp) {
        auto* eMatch  = section.first->asExp();
        auto* caseExp = eMatch->getExpression()->emit(ctx);
        if (caseExp->getType()->isChoice() ||
            (caseExp->getType()->isReference() && caseExp->getType()->asReference()->getSubType()->isChoice())) {
          if (caseExp->getType()->isChoice() && caseExp->isImplicitPointer()) {
            caseVal = ctx->builder.CreateLoad(chTy->getLLVMType(), caseExp->getLLVM());
          } else if (caseExp->getType()->isReference()) {
            caseExp->loadImplicitPointer(ctx->builder);
            caseVal = ctx->builder.CreateLoad(chTy->getLLVMType(), caseExp->getLLVM());
          }
        } else {
          ctx->Error("Expected either the name of a variant of, "
                     "or an expression of type " +
                         ctx->highlightError(chTy->getFullName()),
                     eMatch->getMainRange());
        }
      } else {
        auto* cMatch = section.first->asChoice();
        for (const auto& mField : mentionedFields) {
          if (mField.value == cMatch->getName().value) {
            ctx->Error("The variant " + ctx->highlightError(cMatch->getName().value) + " of choice type " +
                           ctx->highlightError(chTy->getFullName()) +
                           " is repeating here. Please check logic and make "
                           "necessary changes",
                       cMatch->getFileRange());
          }
        }
        mentionedFields.push_back(cMatch->getName());
        if (chTy->hasField(cMatch->getName().value)) {
          caseVal = llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, chTy->getBitwidth()),
                                           (u64)chTy->getValueFor(cMatch->getName().value));
        } else {
          ctx->Error("No variant named " + ctx->highlightError(cMatch->getName().value) + " in choice type " +
                         ctx->highlightError(chTy->getFullName()),
                     cMatch->getName().range);
        }
      }
      auto*      trueBlock  = new IR::Block(ctx->fn, curr);
      IR::Block* falseBlock = nullptr;
      auto*      cond       = ctx->builder.CreateICmpEQ(choiceVal, caseVal);
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        falseBlock = new IR::Block(ctx->fn, curr);
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
        ctx->Error("All possible variants of the choice type are already "
                   "mentioned in the match sentence. No need to use else case "
                   "for the match statement",
                   fileRange);
      }
    } else {
      if (!elseCase.has_value()) {
        ctx->Error("Not all possible variants of the choice type are provided. "
                   "Please add the else case to handle all missing variants",
                   fileRange);
      }
    }
    if (elseCase.has_value()) {
      emitSentences(elseCase.value(), ctx);
      ctx->fn->getBlock()->destroyLocals(ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
    }
    restBlock->setActive(ctx->builder);
  }
  // FIXME - Support Enum types
  else {
  }
  return nullptr;
}

Json Match::toJson() const {
  Vec<JsonValue> chainJson;
  for (const auto& elem : chain) {
    Vec<JsonValue> sentencesJson;
    for (auto* snt : elem.second) {
      sentencesJson.push_back(snt->toJson());
    }
    chainJson.push_back(Json()._("matchValue", elem.first->toJson())._("sentences", sentencesJson));
  }
  Vec<JsonValue> elseJson;
  if (elseCase.has_value()) {
    for (auto* snt : elseCase.value()) {
      elseJson.push_back(snt->toJson());
    }
  }
  return Json()
      ._("candidate", candidate->toJson())
      ._("matchChain", chainJson)
      ._("hasElse", elseCase.has_value())
      ._("elseSentences", elseJson);
}

} // namespace qat::ast