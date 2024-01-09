#include "./say_sentence.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/stdlib.hpp"
#include "../../cli/config.hpp"

namespace qat::ast {

SayLike::SayLike(SayType _sayTy, Vec<Expression*> _expressions, FileRange _fileRange)
    : Sentence(std::move(_fileRange)), expressions(std::move(_expressions)), sayType(_sayTy) {}

IR::Value* SayLike::emit(IR::Context* ctx) {
  auto* cfg = cli::Config::get();
  if ((sayType == SayType::dbg) ? cfg->isDebugMode() : true) {
    SHOW("Say sentence emitting..")
    auto* mod = ctx->getMod();
    SHOW("Current block is: " << ctx->getActiveFunction()->getBlock()->getName())
    mod->linkNative(IR::NativeUnit::printf);
    auto*           printfFn = mod->getLLVMModule()->getFunction("printf");
    Vec<IR::Value*> valuesIR;
    Vec<FileRange>  valuesRange;
    for (usize i = 0; i < expressions.size(); i++) {
      valuesRange.push_back(expressions[i]->fileRange);
      valuesIR.push_back(expressions[i]->emit(ctx));
    }
    auto fmtRes = IR::Logic::formatValues(ctx, valuesIR, {}, fileRange);
    if (sayType != SayType::only) {
      fmtRes.first += "\n";
    }
    Vec<llvm::Value*> values;
    values.push_back(ctx->builder.CreateGlobalStringPtr(fmtRes.first, ctx->getGlobalStringName()));
    for (auto* val : fmtRes.second) {
      values.push_back(val);
    }
    ctx->builder.CreateCall(printfFn->getFunctionType(), printfFn, values);
    return nullptr;
  } else {
    return nullptr;
  }
}

Json SayLike::toJson() const {
  Vec<JsonValue> exps;
  for (auto* exp : expressions) {
    exps.push_back(exp->toJson());
  }
  return Json()._("nodeType", "saySentence")._("values", exps)._("fileRange", fileRange);
}

} // namespace qat::ast