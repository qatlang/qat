#include "./future.hpp"
#include "../../IR/types/future.hpp"

namespace qat::ast {

FutureType::FutureType(bool _isVar, ast::QatType* _subType, utils::FileRange _fileRange)
    : QatType(_isVar, std::move(_fileRange)), subType(_subType) {}

IR::QatType* FutureType::emit(IR::Context* ctx) { return IR::FutureType::get(subType->emit(ctx), ctx); }

TypeKind FutureType::typeKind() const { return TypeKind::future; }

Json FutureType::toJson() const {
  return Json()._("typeKind", "future")._("subType", subType->toJson())._("fileRange", fileRange);
}

String FutureType::toString() const { return (isVariable() ? "var " : "") + subType->toString(); }

} // namespace qat::ast