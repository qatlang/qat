#include "./default.hpp"
#include "../../utils/number_to_position.hpp"
#include "../types/const_generic.hpp"
#include "../types/generic_abstract.hpp"
#include "../types/typed_generic.hpp"

namespace qat::ast {

PrerunDefault::PrerunDefault(Maybe<ast::QatType*> _type, FileRange range)
    : PrerunExpression(std::move(range)), theType(_type) {}

void PrerunDefault::setGenericAbstract(ast::GenericAbstractType* genAbs) const { genericAbstractType = genAbs; }

IR::PrerunValue* PrerunDefault::emit(IR::Context* ctx) {
  if (theType.has_value()) {
    auto* type = theType.value()->emit(ctx);
    if (type->isInteger()) {
      return new IR::PrerunValue(llvm::ConstantInt::get(type->asInteger()->getLLVMType(), 0u), type->asInteger());
    } else if (type->isUnsignedInteger()) {
      return new IR::PrerunValue(llvm::ConstantInt::get(type->asUnsignedInteger()->getLLVMType(), 0u),
                                 type->asUnsignedInteger());
    }
    ctx->Error("Constant " + ctx->highlightError("default") + " expression is currently not supported", fileRange);
  } else if (genericAbstractType.has_value()) {
    auto* genVal = genericAbstractType.value();
    if (genVal->isTyped()) {
      if (genVal->asTyped()->hasDefault()) {
        if (!genVal->isSet()) {
          genVal->emit(ctx);
        }
        return new IR::PrerunValue(IR::TypedType::get(genVal->asTyped()->getDefault()));
      } else {
        ctx->Error(utils::numberToPosition(genVal->getIndex()) + " Generic Type Parameter " +
                       ctx->highlightError(genVal->getName().value) + " doesn't have a default type associated with it",
                   fileRange);
      }
    } else if (genVal->isConst()) {
      // NOTE - The above is not just else, because there might be additional kinds of generic parameters in the future
      // Although it is foolish to consider this possibility in this file, but not in any other file
      if (genVal->asConst()->hasDefault()) {
        if (!genVal->isSet()) {
          genVal->emit(ctx);
        }
        return genVal->asConst()->getDefault();
      } else {
        ctx->Error(utils::numberToPosition(genVal->getIndex()) + " Const Generic Parameter " +
                       ctx->highlightError(genVal->getName().value) +
                       " doesn't have a default constant expression associated with it",
                   fileRange);
      }
    } else {
      ctx->Error("Invalid generic kind", genVal->getRange());
    }
  } else {
    ctx->Error("No type inferred from the context for constant " + ctx->highlightError("default") + " expression",
               fileRange);
  }
  return nullptr;
}

Json PrerunDefault::toJson() const {
  return Json()
      ._("nodeType", "constantDefault")
      ._("hasType", theType.has_value())
      ._("type", theType.has_value() ? theType.value()->toJson() : Json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast