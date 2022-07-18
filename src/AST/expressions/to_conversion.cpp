#include "./to_conversion.hpp"

namespace qat::AST {

IR::Value *ToConversion::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json ToConversion::toJson() const {
  return nuo::Json()
      ._("nodeType", "toConversion")
      ._("instance", source->toJson())
      ._("targetType", destinationType->toJson())
      ._("filePlacement", fileRange);
}

} // namespace qat::AST