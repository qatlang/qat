#include "./tuple.hpp"
#include "../../IR/types/tuple.hpp"
#include <vector>

namespace qat::AST {

TupleType::TupleType(const std::vector<AST::QatType *> _types,
                     const bool _isPacked, const bool _variable,
                     const utils::FileRange _filePlacement)
    : types(_types), isPacked(_isPacked), QatType(_variable, _filePlacement) {}

IR::QatType *TupleType::emit(IR::Context *ctx) {
  std::vector<IR::QatType *> irTypes;
  for (auto &type : types) {
    if (type->typeKind() == AST::TypeKind::Void) {
      ctx->throw_error("Tuple member type cannot be `void`", filePlacement);
    }
    irTypes.push_back(type->emit(ctx));
  }
  return new IR::TupleType(irTypes, isPacked);
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

nuo::Json TupleType::toJson() const {
  std::vector<nuo::JsonValue> mems;
  for (auto mem : types) {
    mems.push_back(mem->toJson());
  }
  return nuo::Json()
      ._("typeKind", "tuple")
      ._("members", mems)
      ._("isVariable", isVariable())
      ._("isPacked", isPacked)
      ._("filePlacement", filePlacement);
}

std::string TupleType::toString() const {
  std::string result;
  if (isVariable()) {
    result = "var (";
  } else {
    result = "(";
  }
  for (std::size_t i = 0; i < types.size(); i++) {
    result += types.at(i)->toString();
    if (i != (types.size() - 1)) {
      result += ", ";
    }
  }
  result += ")";
}

} // namespace qat::AST