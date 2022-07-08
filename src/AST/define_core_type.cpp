#include "./define_core_type.hpp"

namespace qat {
namespace AST {

IR::Value *DefineCoreType::emit(IR::Context *ctx) {
  // FIXME
}

void DefineCoreType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (isHeader) {
    file += "\nclass " + name + " {";
    // FIXME
    file += "};\n";
  } else {
    // FIXME
  }
}

} // namespace AST
} // namespace qat