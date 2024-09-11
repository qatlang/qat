#include "./generics.hpp"
#include "../ast/types/generic_abstract.hpp"
#include "../ast/types/prerun_generic.hpp"
#include "../ast/types/typed_generic.hpp"
#include "../utils/utils.hpp"
#include "./types/typed.hpp"
#include "./value.hpp"

namespace qat::ir {

void fill_generics(ir::Ctx* irCtx, Vec<ast::GenericAbstractType*>& generics, Vec<GenericToFill*>& types,
                   FileRange const& fileRange) {
  for (usize i = 0; i < generics.size(); i++) {
    if (generics.at(i)->is_typed()) {
      if ((types.size() > i)) {
        if (types.at(i)->is_type()) {
          generics.at(i)->as_typed()->setType(types.at(i)->as_type());
        } else {
          if (types.at(i)->as_prerun()->get_ir_type()->is_typed()) {
            generics.at(i)->as_typed()->setType(types.at(i)->as_prerun()->get_ir_type()->as_typed()->get_subtype());
          } else {
            irCtx->Error(
                utils::number_to_position(i + 1) + " Generic Parameter " +
                    irCtx->color(generics.at(i)->get_name().value) +
                    " expects a type or a constant expression that gives a type, but a normal constant expression was provided",
                fileRange);
          }
        }
      } else {
        if (generics.at(i)->isSet()) {
          types.push_back(
              ir::GenericToFill::GetType(generics.at(i)->as_typed()->get_type(), generics.at(i)->get_range()));
        } else {
          irCtx->Error("No type set for " + utils::number_to_position(i + 1) + " Generic Parameter " +
                           irCtx->color(generics.at(i)->get_name().value) +
                           " and it doesn't have a default type associated with it",
                       generics.at(i)->get_range());
        }
      }
    } else {
      if (types.size() > i) {
        if (types.at(i)->is_prerun() || generics.at(i)->as_prerun()->getType()->is_typed()) {
          if (types.at(i)->is_prerun()) {
            generics.at(i)->as_prerun()->setExpression(types.at(i)->as_prerun());
          } else {
            generics.at(i)->as_prerun()->setExpression(
                ir::PrerunValue::get_typed_prerun(ir::TypedType::get(types.at(i)->as_type())));
          }
        } else {
          irCtx->Error(utils::number_to_position(i + 1) + " Generic Parameter " +
                           irCtx->color(generics.at(i)->get_name().value) +
                           " expects a constant expression, not a type",
                       fileRange);
        }
      } else {
        if (generics.at(i)->isSet()) {
          types.push_back(
              ir::GenericToFill::GetPrerun(generics.at(i)->as_prerun()->getPrerun(), generics.at(i)->get_range()));
        } else {
          irCtx->Error("No const expression set for " + utils::number_to_position(i + 1) + " Generic Parameter " +
                           irCtx->color(generics.at(i)->get_name().value) +
                           " and it doesn't have a default const expression associated with it",
                       generics.at(i)->get_range());
        }
      }
    }
  }
}

GenericToFill::GenericToFill(void* _data, GenericKind _kind, FileRange _range)
    : data(_data), kind(_kind), range(std::move(_range)) {}

GenericToFill* GenericToFill::GetPrerun(ir::PrerunValue* constVal, FileRange _range) {
  return new GenericToFill(constVal, GenericKind::prerunGeneric, std::move(_range));
}

GenericToFill* GenericToFill::GetType(ir::Type* type, FileRange _range) {
  SHOW("GenericToFill of type is created")
  SHOW("Type is: " << type)
  SHOW("TypeKind is: " << (int)type->type_kind());
  return new GenericToFill(type, GenericKind::typedGeneric, std::move(_range));
}

bool GenericToFill::is_type() const { return kind == GenericKind::typedGeneric; }

ir::Type* GenericToFill::as_type() const { return (ir::Type*)data; }

bool GenericToFill::is_prerun() const { return kind == GenericKind::prerunGeneric; }

ir::PrerunValue* GenericToFill::as_prerun() const { return (ir::PrerunValue*)data; }

FileRange GenericToFill::get_range() const { return range; }

String GenericToFill::to_string() const {
  if (is_type()) {
    return as_type()->to_string();
  } else {
    return as_prerun()->get_ir_type()->to_prerun_generic_string(as_prerun()).value();
  }
}

GenericArgument::GenericArgument(Identifier _name, GenericKind _kind, FileRange _range)
    : name(std::move(_name)), kind(_kind), range(std::move(_range)) {}

Identifier GenericArgument::get_name() const { return name; }

FileRange GenericArgument::get_range() const { return range; }

bool GenericArgument::is_same(const String& cName) const { return name.value == cName; }

bool GenericArgument::is_equal_to(ir::Ctx* irCtx, GenericToFill* fill) const {
  if (is_typed()) {
    if (fill->is_type()) {
      return as_typed()->get_type()->is_same(fill->as_type());
    } else {
      if (fill->as_prerun()->get_ir_type()->is_typed()) {
        return as_typed()->get_type()->is_same(fill->as_prerun()->get_ir_type()->as_typed()->get_subtype());
      } else {
        return false;
      }
    }
  } else {
    if (fill->is_type()) {
      if (as_prerun()->get_type()->is_typed()) {
        return as_prerun()->get_type()->as_typed()->is_same(fill->as_type());
      } else {
        return false;
      }
    } else {
      return fill->as_prerun()->is_equal_to(irCtx, as_prerun()->get_expression());
    }
  }
}

bool GenericArgument::is_typed() const { return kind == GenericKind::typedGeneric; }

TypedGeneric* GenericArgument::as_typed() const { return (TypedGeneric*)this; }

bool GenericArgument::is_prerun() const { return kind == GenericKind::prerunGeneric; }

PrerunGeneric* GenericArgument::as_prerun() const { return (PrerunGeneric*)this; }

String GenericArgument::to_string() const {
  if (is_prerun()) {
    return as_prerun()
        ->get_expression()
        ->get_ir_type()
        ->to_prerun_generic_string(as_prerun()->get_expression())
        .value();
  } else if (is_typed()) {
    return as_typed()->get_type()->to_string();
  } else {
    // FIXME - Do something here
  }
}

TypedGeneric::TypedGeneric(Identifier _name, ir::Type* _type, FileRange _range)
    : GenericArgument(std::move(_name), GenericKind::typedGeneric, std::move(_range)), type(_type) {}

TypedGeneric* TypedGeneric::get(Identifier name, ir::Type* type, FileRange range) {
  return new TypedGeneric(std::move(name), type, std::move(range));
}

ir::Type* TypedGeneric::get_type() const { return type; }

Json TypedGeneric::to_json() const {
  return Json()
      ._("name", name)
      ._("genericKind", "typedGeneric")
      ._("typeID", type->get_id())
      ._("valueString", type->to_string())
      ._("range", range);
}

PrerunGeneric::PrerunGeneric(Identifier _name, ir::PrerunValue* _val, FileRange _range)
    : GenericArgument(std::move(_name), GenericKind::prerunGeneric, std::move(_range)), constant(_val) {}

PrerunGeneric* PrerunGeneric::get(Identifier name, ir::PrerunValue* val, FileRange range) {
  return new PrerunGeneric(std::move(name), val, std::move(range));
}

ir::PrerunValue* PrerunGeneric::get_expression() const { return constant; }

ir::Type* PrerunGeneric::get_type() const { return constant->get_ir_type(); };

Json PrerunGeneric::to_json() const {
  return Json()
      ._("name", name)
      ._("genericKind", "prerunGeneric")
      ._("typeID", constant->get_ir_type()->get_id())
      ._("valueString", constant->get_ir_type()->can_be_prerun_generic()
                            ? constant->get_ir_type()->to_prerun_generic_string(constant).value_or("")
                            : "")
      ._("range", range);
}

} // namespace qat::ir
