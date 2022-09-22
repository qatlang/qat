#include "./match.hpp"
#include "../../IR/control_flow.hpp"

namespace qat::ast {

UnionMatchValue      *MatchValue::asUnion() { return (UnionMatchValue *)this; }
EnumMatchValue       *MatchValue::asEnum() { return (EnumMatchValue *)this; }
ExpressionMatchValue *MatchValue::asExp() {
  return (ExpressionMatchValue *)this;
}

UnionMatchValue::UnionMatchValue(
    Pair<String, utils::FileRange>        _name,
    Maybe<Pair<String, utils::FileRange>> _valueName, bool _isVar)
    : name(std::move(_name)), valueName(std::move(_valueName)), isVar(_isVar) {}

String UnionMatchValue::getName() const { return name.first; }

utils::FileRange UnionMatchValue::getNameRange() const { return name.second; }

bool UnionMatchValue::hasValueName() const { return valueName.has_value(); }

bool UnionMatchValue::isVariable() const { return isVar; }

String UnionMatchValue::getValueName() const { return valueName->first; }

utils::FileRange UnionMatchValue::getValueRange() const {
  return valueName->second;
}

nuo::Json UnionMatchValue::toJson() const {
  return nuo::Json()
      ._("matchValueType", "union")
      ._("name", name.first)
      ._("nameFileRange", name.second)
      ._("hasValue", valueName.has_value())
      ._("valueName", valueName.has_value() ? valueName->first : "")
      ._("valueFileRange",
         valueName.has_value() ? valueName->second : nuo::JsonValue());
}

EnumMatchValue::EnumMatchValue(Pair<String, utils::FileRange> _name)
    : name(std::move(_name)) {}

String EnumMatchValue::getName() const { return name.first; }

utils::FileRange EnumMatchValue::getFileRange() const { return name.second; }

nuo::Json EnumMatchValue::toJson() const {
  return nuo::Json()
      ._("matchValueType", "enum")
      ._("name", name.first)
      ._("nameFileRange", name.second);
}

Expression *ExpressionMatchValue::getExpression() const { return exp; }

nuo::Json ExpressionMatchValue::toJson() const {
  return nuo::Json()
      ._("matchValueType", "expression")
      ._("expression", exp->toJson());
}

Match::Match(bool _isTypeMatch, Expression *_candidate,
             Vec<Pair<MatchValue *, Vec<Sentence *>>> _chain,
             Maybe<Vec<Sentence *>> _elseCase, utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), isTypeMatch(_isTypeMatch),
      candidate(_candidate), chain(std::move(_chain)),
      elseCase(std::move(_elseCase)) {}

IR::Value *Match::emit(IR::Context *ctx) {
  auto *expEmit       = candidate->emit(ctx);
  auto *expTy         = expEmit->getType();
  bool  isExpVariable = false;
  if (expTy->isReference()) {
    expEmit->loadImplicitPointer(ctx->builder);
    isExpVariable = expTy->asReference()->isSubtypeVariable();
    expTy         = expTy->asReference()->getSubType();
  } else {
    isExpVariable = expEmit->isVariable();
  }
  auto *curr      = ctx->fn->getBlock();
  auto *restBlock = new IR::Block(ctx->fn, curr->getParent());
  if (isTypeMatch && expTy->isUnion()) {
    auto *uTy = expTy->asUnion();
    if (!expEmit->isReference() && !expEmit->isImplicitPointer()) {
      ctx->Error("Invalid value for matching", candidate->fileRange);
    }
    Vec<String> mentionedFields;
    for (usize i = 0; i < chain.size(); i++) {
      const auto &section = chain.at(i);
      auto       *uMatch  = section.first->asUnion();
      if (uMatch->isVariable()) {
        if (!isExpVariable) {
          ctx->Error(
              "The expression being matched does not possess variability and "
              "hence the matched "
              "case cannot use a variable reference to the subtype value",
              uMatch->getValueRange());
        }
      }
      if (section.first->getType() != MatchType::Union) {
        ctx->Error("Expected name of the sub-field of the union type " +
                       ctx->highlightError(expTy->asUnion()->getFullName()),
                   uMatch->getNameRange());
      }
      auto subRes = uTy->hasSubTypeWithName(uMatch->getName());
      if (!subRes.first) {
        ctx->Error("No field named " + ctx->highlightError(uMatch->getName()) +
                       " in union type " +
                       ctx->highlightError(uTy->getFullName()),
                   uMatch->getNameRange());
      }
      if (!subRes.second && uMatch->hasValueName()) {
        ctx->Error(
            "Sub-field " + ctx->highlightError(uMatch->getName()) +
                " of union type " + ctx->highlightError(uTy->getFullName()) +
                " does not have a type associated with it, and hence you "
                "cannot use a name for the value here",
            uMatch->getValueRange());
      }
      for (usize j = i + 1; j < chain.size(); j++) {
        if (uMatch->getName() == chain.at(j).first->asUnion()->getName()) {
          ctx->Error("Repeating subfield of the union type " +
                         ctx->highlightError(uTy->getFullName()),
                     chain.at(j).first->asUnion()->getNameRange());
        }
      }
      mentionedFields.push_back(uMatch->getName());
      auto      *trueBlock  = new IR::Block(ctx->fn, curr);
      IR::Block *falseBlock = nullptr;
      auto      *cond       = ctx->builder.CreateICmpEQ(
          ctx->builder.CreateLoad(
              llvm::Type::getIntNTy(ctx->llctx, uTy->getTagBitwidth()),
              ctx->builder.CreateStructGEP(uTy->getLLVMType(),
                                                      expEmit->getLLVM(), 0)),
          llvm::ConstantInt::get(
              llvm::Type::getIntNTy(ctx->llctx, uTy->getTagBitwidth()),
              uTy->getIndexOfName(uMatch->getName())));
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        falseBlock = new IR::Block(ctx->fn, curr);
        ctx->builder.CreateCondBr(cond, trueBlock->getBB(),
                                  falseBlock->getBB());
      } else {
        ctx->builder.CreateCondBr(cond, trueBlock->getBB(), restBlock->getBB());
      }
      trueBlock->setActive(ctx->builder);
      SHOW("True block name is: " << trueBlock->getName())
      if (section.first->asUnion()->hasValueName()) {
        SHOW("Match case has value name: "
             << section.first->asUnion()->getValueName())
        if (trueBlock->hasValue(uMatch->getValueName())) {
          SHOW("Local entity with match case value name exists")
          ctx->Error("Local entity named " +
                         ctx->highlightError(
                             section.first->asUnion()->getValueName()) +
                         " exists already in this scope",
                     section.first->asUnion()->getValueRange());
        } else {
          SHOW("Creating local entity for match case value named: "
               << uMatch->getValueName())
          auto *loc = trueBlock->newValue(
              uMatch->getValueName(),
              IR::ReferenceType::get(uMatch->isVariable(),
                                     uTy->getSubTypeWithName(uMatch->getName()),
                                     ctx->llctx),
              false);
          SHOW("Local Entity for match case created")
          ctx->builder.CreateStore(
              ctx->builder.CreatePointerCast(
                  ctx->builder.CreateStructGEP(uTy->getLLVMType(),
                                               expEmit->getLLVM(), 1),
                  uTy->getSubTypeWithName(uMatch->getName())
                      ->getLLVMType()
                      ->getPointerTo()),
              loc->getAlloca());
        }
      }
      emitSentences(section.second, ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
      if (i == (chain.size() - 1) ? elseCase.has_value() : true) {
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        falseBlock->setActive(ctx->builder);
      }
    }
    Vec<String> missingFields;
    uTy->getMissingNames(mentionedFields, missingFields);
    if (missingFields.empty()) {
      if (elseCase.has_value()) {
        ctx->Error("All possible subtypes of the union are already mentioned. "
                   "No need to use else case for the match statement",
                   fileRange);
      }
    } else {
      if (!elseCase.has_value()) {
        ctx->Error(
            "Not all possible subtypes of the union are provided. Please add "
            "the else case to handle all missing possibilities",
            fileRange);
      }
    }
    if (elseCase.has_value()) {
      emitSentences(elseCase.value(), ctx);
      (void)IR::addBranch(ctx->builder, restBlock->getBB());
    }
    restBlock->setActive(ctx->builder);
  }
  // FIXME - Support Enum types
  else {
  }
  return nullptr;
}

nuo::Json Match::toJson() const {
  Vec<nuo::JsonValue> chainJson;
  for (const auto &elem : chain) {
    Vec<nuo::JsonValue> sentencesJson;
    for (auto *snt : elem.second) {
      sentencesJson.push_back(snt->toJson());
    }
    chainJson.push_back(nuo::Json()
                            ._("matchValue", elem.first->toJson())
                            ._("sentences", sentencesJson));
  }
  Vec<nuo::JsonValue> elseJson;
  if (elseCase.has_value()) {
    for (auto *snt : elseCase.value()) {
      elseJson.push_back(snt->toJson());
    }
  }
  return nuo::Json()
      ._("candidate", candidate->toJson())
      ._("matchChain", chainJson)
      ._("hasElse", elseCase.has_value())
      ._("elseSentences", elseJson);
}

} // namespace qat::ast