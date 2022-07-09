#include "./to_conversion.hpp"

namespace qat {
namespace AST {

IR::Value *ToConversion::emit(IR::Context *ctx) {
  // TODO - Implement this
}

backend::JSON ToConversion::toJSON() const {
  return backend::JSON()
      ._("nodeType", "toConversion")
      ._("instance", source->toJSON())
      ._("targetType", destinationType->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat