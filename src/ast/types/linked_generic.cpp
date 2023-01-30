#include "./linked_generic.hpp"
#include "../../utils/number_to_position.hpp"
#include "const_generic.hpp"
#include "generic_abstract.hpp"
#include "type_kind.hpp"
#include "typed_generic.hpp"

namespace qat::ast {

LinkedGeneric::LinkedGeneric(bool _isVariable, ast::GenericAbstractType* _genAbs, FileRange range)
    : QatType(_isVariable, std::move(range)), genAbs(_genAbs) {}

IR::QatType* LinkedGeneric::emit(IR::Context* ctx) {
  if (genAbs->isTyped() || (genAbs->isConst() && genAbs->asConst()->getType()->isTyped())) {
    if (genAbs->isSet()) {
      if (genAbs->isTyped()) {
        return genAbs->asTyped()->getType();
      } else if (genAbs->isConst()) {
        return genAbs->asConst()->getConstant()->getType()->asTyped()->getSubType();
      } else {
        ctx->Error("Invalid generic kind", fileRange);
      }
    } else {
      if (genAbs->isTyped()) {
        ctx->Error("No type set for the " + utils::numberToPosition(genAbs->getIndex()) + " Generic Parameter " +
                       ctx->highlightError(genAbs->getName().value),
                   fileRange);
      } else if (genAbs->isConst()) {
        ctx->Error("No constant expression set for the " + utils::numberToPosition(genAbs->getIndex()) +
                       " Generic Parameter " + ctx->highlightError(genAbs->getName().value),
                   fileRange);
      } else {
        ctx->Error("Invalid generic kind", fileRange);
      }
    }
  } else if (genAbs->isConst()) {
    ctx->Error(utils::numberToPosition(genAbs->getIndex()) + " Generic Parameter " +
                   ctx->highlightError(genAbs->getName().value) +
                   " is a normal constant expression and hence cannot be used as a type",
               fileRange);
  } else {
    ctx->Error("Invalid generic kind", fileRange);
  }
}

TypeKind LinkedGeneric::typeKind() const { return TypeKind::linkedGeneric; }

String LinkedGeneric::toString() const { return (isVariable() ? "var " : "") + genAbs->getName().value; }

Json LinkedGeneric::toJson() const {
  return Json()._("isVariable", isVariable())._("genericParameter", genAbs->getName())._("fileRange", fileRange);
}

} // namespace qat::ast