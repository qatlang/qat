#include "./expose_box.hpp"

namespace qat {
namespace AST {

llvm::Value *ExposeBoxes::generate(IR::Generator *generator) {
  std::size_t count = 0;
  for (auto existing : generator->exposed) {
    for (auto candidate : boxes) {
      if (existing.generate() == candidate.generate()) {
        Errors::AST4(candidate.generate(), (boxes.size() > 1), file_placement);
        break;
        // NOTE - This break is currently unnecessary, but in the future, when
        // compilation is not stopped on the first encounter of an error,
        // this is needed
      } else {
        generator->exposed.push_back(candidate);
        count++;
      }
    }
  }
  for (auto sentence : sentences) {
    sentence.generate(generator);
  }
  /**
   *  Interating both vectors in one loop instead of two as this makes
   * more sense for this scenario
   *
   */
  auto exp_size = generator->exposed.size();
  auto boxes_size = boxes.size();
  for (auto i = exp_size - 1; ((i >= 0) && (i >= (exp_size - count))); i--) {
    if (generator->exposed.at(i).generate() ==
        boxes.at(boxes_size - (exp_size - i)).generate()) {
      generator->exposed.pop_back();
    }
  }
  return nullptr;
}

backend::JSON ExposeBoxes::toJSON() const {
  std::vector<backend::JsonValue> boxs;
  std::vector<backend::JSON> sntcs;
  for (auto box : boxes) {
    boxs.push_back(backend::JsonValue(box.generate()));
  }
  for (auto sen : sentences) {
    sntcs.push_back(sen.toJSON());
  }
  return backend::JSON()
      ._("nodeType", "exposeBoxes")
      ._("boxes", boxs)
      ._("sentences", sntcs)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat