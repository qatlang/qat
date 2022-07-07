#include "./tuple.hpp"
#include "../../IR/types/tuple.hpp"
#include <vector>

namespace qat {
namespace AST {

TupleType::TupleType(const std::vector<AST::QatType *> _types,
                     const bool _isPacked, const bool _variable,
                     const utils::FilePlacement _filePlacement)
    : types(_types), isPacked(_isPacked), QatType(_variable, _filePlacement) {}

IR::QatType *TupleType::emit(IR::Context *ctx) {
  std::vector<IR::QatType *> irTypes;
  for (auto &type : types) {
    if (type->typeKind() == AST::TypeKind::Void) {
      ctx->throw_error("Tuple member type cannot be `void`", filePlacement);
    }
    irTypes.push_back(type->emit(ctx));
  }
  return new IR::TupleType(ctx->llvmContext, irTypes, isPacked);
}

void TupleType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  file.addInclude("<tuple>");
  if (isConstant()) {
    file += "const ";
  }
  file += "std::tuple<";
  for (std::size_t i = 0; i < types.size(); i++) {
    types.at(i)->emitCPP(file, isHeader);
    if (i != (types.size() - 1)) {
      file += ", ";
    }
  }
  file += "> ";
}

TypeKind TupleType::typeKind() { return TypeKind::tuple; }

backend::JSON TupleType::toJSON() const {
  std::vector<backend::JSON> mems;
  for (auto mem : types) {
    mems.push_back(mem->toJSON());
  }
  return backend::JSON()
      ._("typeKind", "tuple")
      ._("members", mems)
      ._("isVariable", isVariable())
      ._("isPacked", isPacked)
      ._("filePlacement", filePlacement);
}

} // namespace AST
} // namespace qat