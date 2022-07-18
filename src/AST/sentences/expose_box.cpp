#include "./expose_box.hpp"

namespace qat::AST {

IR::Value *ExposeBoxes::emit(IR::Context *ctx) {
  std::size_t count = 0;
  for (auto existing : ctx->exposed) {
    for (auto candidate : boxes) {
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
   *  Interating both vectors in one loop instead of two as this makes
   * more sense for this scenario
   *
   */
  auto exp_size = ctx->exposed.size();
  auto boxes_size = boxes.size();
  for (auto i = exp_size - 1; ((i >= 0) && (i >= (exp_size - count))); i--) {
    if (ctx->exposed.at(i) == boxes.at(boxes_size - (exp_size - i))) {
      ctx->exposed.pop_back();
    }
  }
  return nullptr;
}

void ExposeBoxes::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "{\n";
    for (auto box : boxes) {
      // TODO - Resolve relative named boxes
      file += "   using namespace " + box + ";\n";
    }
    for (auto snt : sentences) {
      snt->emitCPP(file, isHeader);
    }
    file += "}\n";
  }
}

nuo::Json ExposeBoxes::toJson() const {
  std::vector<nuo::JsonValue> boxs;
  std::vector<nuo::JsonValue> sntcs;
  for (auto box : boxes) {
    boxs.push_back(nuo::JsonValue(box));
  }
  for (auto sen : sentences) {
    sntcs.push_back(sen->toJson());
  }
  return nuo::Json()
      ._("nodeType", "exposeBoxes")
      ._("boxes", boxs)
      ._("sentences", sntcs)
      ._("filePlacement", fileRange);
}

} // namespace qat::AST