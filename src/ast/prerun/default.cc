#include "./default.hpp"
#include "../../utils/utils.hpp"
#include "../types/generic_abstract.hpp"
#include "../types/prerun_generic.hpp"
#include "../types/typed_generic.hpp"

namespace qat::ast {

void PrerunDefault::setGenericAbstract(ast::GenericAbstractType* genAbs) const { genericAbstractType = genAbs; }

String PrerunDefault::to_string() const {
  return "default" + String(theType.has_value() ? (":[" + theType.value()->to_string() + "]") : "");
}

ir::PrerunValue* PrerunDefault::emit(EmitCtx* ctx) {
  if (theType.has_value() || is_type_inferred()) {
    auto* type = theType.has_value() ? theType.value()->emit(ctx) : inferredType;
    if (type->is_integer()) {
      return ir::PrerunValue::get(llvm::ConstantInt::get(type->as_integer()->get_llvm_type(), 0u), type->as_integer());
    } else if (type->is_unsigned_integer()) {
      return ir::PrerunValue::get(llvm::ConstantInt::get(type->as_unsigned_integer()->get_llvm_type(), 0u),
                                  type->as_unsigned_integer());
    } else if (type->is_choice() && type->as_choice()->has_default()) {
      return ir::PrerunValue::get(type->as_choice()->get_default(), type);
    } else if (type->has_prerun_default_value()) {
      return type->get_prerun_default_value(ctx->irCtx);
    }
    ctx->Error("Type " + ctx->color(type->to_string()) + " does not have a prerun " + ctx->color("default") + " value",
               fileRange);
  } else if (genericAbstractType.has_value()) {
    auto* genVal = genericAbstractType.value();
    if (genVal->is_typed()) {
      if (genVal->as_typed()->hasDefault()) {
        if (!genVal->isSet()) {
          genVal->emit(ctx);
        }
        return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(genVal->as_typed()->getDefault()));
      } else {
        ctx->Error(utils::number_to_position(genVal->getIndex()) + " Generic Type Parameter " +
                       ctx->color(genVal->get_name().value) + " doesn't have a default type associated with it",
                   fileRange);
      }
    } else if (genVal->as_typed()) {
      // NOTE - The above is not just else, because there might be additional kinds of generic parameters in the future
      // Although it is foolish to consider this possibility in this file, but not in any other file
      if (genVal->as_prerun()->hasDefault()) {
        if (!genVal->isSet()) {
          genVal->emit(ctx);
        }
        return genVal->as_prerun()->getDefault();
      } else {
        ctx->Error(utils::number_to_position(genVal->getIndex()) + " Prerun Generic Parameter " +
                       ctx->color(genVal->get_name().value) +
                       " doesn't have a default prerun expression associated with it",
                   fileRange);
      }
    } else {
      ctx->Error("Invalid generic kind", genVal->get_range());
    }
  } else {
    ctx->Error("No type inferred from the scope for prerun " + ctx->color("default") + " expression", fileRange);
  }
  return nullptr;
}

Json PrerunDefault::to_json() const {
  return Json()
      ._("nodeType", "prerunDefault")
      ._("hasType", theType.has_value())
      ._("type", theType.has_value() ? theType.value()->to_json() : Json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast