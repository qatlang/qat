#include "./linked_generic.hpp"
#include "../../utils/utils.hpp"
#include "generic_abstract.hpp"
#include "prerun_generic.hpp"
#include "type_kind.hpp"
#include "typed_generic.hpp"

namespace qat::ast {

IR::QatType* LinkedGeneric::emit(IR::Context* ctx) {
  if (genAbs->isTyped() || (genAbs->isPrerun() && genAbs->asPrerun()->getType()->isTyped())) {
    if (genAbs->isSet()) {
      if (genAbs->isTyped()) {
        return genAbs->asTyped()->getType();
      } else if (genAbs->isPrerun()) {
        return genAbs->asPrerun()->getPrerun()->getType()->asTyped()->getSubType();
      } else {
        ctx->Error("Invalid generic kind", fileRange);
      }
    } else {
      if (genAbs->isTyped()) {
        ctx->Error("No type set for the " + utils::number_to_position(genAbs->getIndex()) + " Generic Parameter " +
                       ctx->highlightError(genAbs->getName().value),
                   fileRange);
      } else if (genAbs->isPrerun()) {
        ctx->Error("No prerun expression set for the " + utils::number_to_position(genAbs->getIndex()) +
                       " Generic Parameter " + ctx->highlightError(genAbs->getName().value),
                   fileRange);
      } else {
        ctx->Error("Invalid generic kind", fileRange);
      }
    }
  } else if (genAbs->isPrerun()) {
    ctx->Error(utils::number_to_position(genAbs->getIndex()) + " Generic Parameter " +
                   ctx->highlightError(genAbs->getName().value) + " is a normal prerun expression with type " +
                   genAbs->asPrerun()->getType()->toString() + " and hence cannot be used as a type",
               fileRange);
  } else {
    ctx->Error("Invalid generic kind", fileRange);
  }
}

AstTypeKind LinkedGeneric::typeKind() const { return AstTypeKind::LINKED_GENERIC; }

String LinkedGeneric::toString() const { return genAbs->getName().value; }

Json LinkedGeneric::toJson() const { return Json()._("genericParameter", genAbs->getName())._("fileRange", fileRange); }

} // namespace qat::ast