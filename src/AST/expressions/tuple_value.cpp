#include "./tuple_value.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat {
namespace AST {

TupleValue::TupleValue(std::vector<Expression *> _members,
                       utils::FilePlacement _filePlacement)
    : members(_members), Expression(_filePlacement) {}

llvm::Value *TupleValue::emit(IR::Generator *generator) {
  std::vector<llvm::Type *> memTypes;
  std::vector<llvm::Value *> memValues;
  for (auto mem : members) {
    auto memVal = mem->emit(generator);
    memValues.push_back(memVal);
    memTypes.push_back(memVal->getType());
  }
  auto tupleType = llvm::StructType::create(memTypes);
  auto currBB = generator->builder.GetInsertBlock();
  auto bb = &currBB->getParent()->getEntryBlock();
  generator->builder.SetInsertPoint(bb);
  auto tupleAlloca = generator->builder.CreateAlloca(tupleType);
  for (std::size_t i = 0; i < memValues.size(); i++) {
    generator->builder.CreateInsertValue(tupleAlloca, memValues.at(i),
                                         {0u, (unsigned int)i});
  }
  return generator->builder.CreateLoad(tupleAlloca->getAllocatedType(),
                                       tupleAlloca, "");
}

void TupleValue::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file.addInclude("<tuple>");
    file += "std::tuple(";
    for (std::size_t i = 0; i < members.size(); i++) {
      members.at(i)->emitCPP(file, isHeader);
      if (i != (members.size() - 1)) {
        file += ", ";
      }
    }
    file += ") ";
  }
}

backend::JSON TupleValue::toJSON() const {
  std::vector<backend::JSON> mems;
  for (auto mem : members) {
    mems.push_back(mem->toJSON());
  }
  return backend::JSON()
      ._("nodeType", "tupleValue")
      ._("members", mems)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat