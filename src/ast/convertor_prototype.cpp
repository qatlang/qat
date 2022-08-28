#include "./convertor_prototype.hpp"
#include "../show.hpp"
#include "llvm/IR/GlobalValue.h"
#include <vector>

namespace qat::ast {

ConvertorPrototype::ConvertorPrototype(bool _isFrom, String _argName,
                                       QatType                *_candidateType,
                                       utils::VisibilityKind   _visibility,
                                       const utils::FileRange &_fileRange)
    : Node(_fileRange), argName(std::move(_argName)),
      candidateType(_candidateType), visibility(_visibility), isFrom(_isFrom) {}

ConvertorPrototype *
ConvertorPrototype::From(const String &_argName, QatType *_candidateType,
                         utils::VisibilityKind   _visibility,
                         const utils::FileRange &_fileRange) {
  return new ConvertorPrototype(true, _argName, _candidateType, _visibility,
                                _fileRange);
}

ConvertorPrototype *ConvertorPrototype::To(QatType              *_candidateType,
                                           utils::VisibilityKind _visibility,
                                           const utils::FileRange &_fileRange) {
  return new ConvertorPrototype(false, "", _candidateType, _visibility,
                                _fileRange);
}

void ConvertorPrototype::setCoreType(IR::CoreType *_coreType) {
  coreType = _coreType;
}

IR::Value *ConvertorPrototype::emit(IR::Context *ctx) {
  if (!coreType) {
    ctx->Error("No core type found for this member function", fileRange);
  }
  IR::MemberFunction *function;
  SHOW("Generating candidate type")
  auto *candidate = candidateType->emit(ctx);
  SHOW("Candidate type generated")
  SHOW("About to create convertor")
  if (isFrom) {
    SHOW("Convertor is FROM")
    function = IR::MemberFunction::CreateFromConvertor(
        coreType, candidate, argName, fileRange, ctx->getVisibInfo(visibility),
        ctx->llctx);
  } else {
    SHOW("Convertor is TO")
    function = IR::MemberFunction::CreateToConvertor(
        coreType, candidate, fileRange, ctx->getVisibInfo(visibility),
        ctx->llctx);
  }
  SHOW("Function created!!")
  // TODO - Set calling convention
  return function;
}

nuo::Json ConvertorPrototype::toJson() const {
  return nuo::Json()
      ._("nodeType", "convertorPrototype")
      ._("isFrom", isFrom)
      ._("argumentName", argName)
      ._("candidateType", candidateType->toJson())
      ._("visibility", utils::kindToJsonValue(visibility));
}

} // namespace qat::ast