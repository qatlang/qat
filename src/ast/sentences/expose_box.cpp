#include "./expose_box.hpp"

namespace qat::ast {

IR::Value *ExposeBoxes::emit(IR::Context *ctx) {
  usize count = 0;
  for (auto const &existing : ctx->exposed) {
    for (auto const &candidate : boxes) {
      if (existing == candidate) {
        Errors::AST4(candidate, (boxes.size() > 1), fileRange);
        break;
        // NOTE - This break is currently unnecessary, but in the future, when
        // compilation is not stopped on the first encounter of an error,
        // this is needed
      } else {
        ctx->exposed.push_back(candidate);
        count++;
      }
    }
  }
  for (auto sentence : sentences) {
    sentence->emit(ctx);
  }
  /**
   *  Interacting both vectors in one loop instead of two as this makes
   * more sense for this scenario
   *
   */
  auto exp_size   = ctx->exposed.size();
  auto boxes_size = boxes.size();
  for (auto i = exp_size - 1; (i >= (exp_size - count)); i--) {
    if (ctx->exposed.at(i) == boxes.at(boxes_size - (exp_size - i))) {
      ctx->exposed.pop_back();
    }
  }
  return nullptr;
}

nuo::Json ExposeBoxes::toJson() const {
  Vec<nuo::JsonValue> boxesJsonValue;
  Vec<nuo::JsonValue> sentencesJsonValue;
  for (auto const &box : boxes) {
    boxesJsonValue.emplace_back(nuo::JsonValue(box));
  }
  for (auto sen : sentences) {
    sentencesJsonValue.emplace_back(sen->toJson());
  }
  return nuo::Json()
      ._("nodeType", "exposeBoxes")
      ._("boxes", boxesJsonValue)
      ._("sentences", sentencesJsonValue)
      ._("fileRange", fileRange);
}

} // namespace qat::ast