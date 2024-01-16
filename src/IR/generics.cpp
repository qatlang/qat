#include "./generics.hpp"
#include "../ast/types/generic_abstract.hpp"
#include "../ast/types/prerun_generic.hpp"
#include "../ast/types/typed_generic.hpp"
#include "../utils/utils.hpp"
#include "./types/typed.hpp"
#include "./value.hpp"

namespace qat::IR {

void fillGenerics(IR::Context* ctx, Vec<ast::GenericAbstractType*>& generics, Vec<GenericToFill*>& types,
                  FileRange const& fileRange) {
  for (usize i = 0; i < generics.size(); i++) {
    if (generics.at(i)->isTyped()) {
      if ((types.size() > i)) {
        if (types.at(i)->isType()) {
          generics.at(i)->asTyped()->setType(types.at(i)->asType());
        } else {
          if (types.at(i)->asPrerun()->getType()->isTyped()) {
            generics.at(i)->asTyped()->setType(types.at(i)->asPrerun()->getType()->asTyped()->getSubType());
          } else {
            ctx->Error(
                utils::number_to_position(i + 1) + " Generic Parameter " +
                    ctx->highlightError(generics.at(i)->getName().value) +
                    " expects a type or a constant expression that gives a type, but a normal constant expression was provided",
                fileRange);
          }
        }
      } else {
        if (generics.at(i)->isSet()) {
          types.push_back(IR::GenericToFill::GetType(generics.at(i)->asTyped()->getType(), generics.at(i)->getRange()));
        } else {
          ctx->Error("No type set for " + utils::number_to_position(i + 1) + " Generic Parameter " +
                         ctx->highlightError(generics.at(i)->getName().value) +
                         " and it doesn't have a default type associated with it",
                     generics.at(i)->getRange());
        }
      }
    } else {
      if (types.size() > i) {
        if (types.at(i)->isPrerun() || generics.at(i)->asPrerun()->getType()->isTyped()) {
          if (types.at(i)->isPrerun()) {
            generics.at(i)->asPrerun()->setExpression(types.at(i)->asPrerun());
          } else {
            generics.at(i)->asPrerun()->setExpression(new IR::PrerunValue(IR::TypedType::get(types.at(i)->asType())));
          }
        } else {
          ctx->Error(utils::number_to_position(i + 1) + " Generic Parameter " +
                         ctx->highlightError(generics.at(i)->getName().value) +
                         " expects a constant expression, not a type",
                     fileRange);
        }
      } else {
        if (generics.at(i)->isSet()) {
          types.push_back(
              IR::GenericToFill::GetPrerun(generics.at(i)->asPrerun()->getPrerun(), generics.at(i)->getRange()));
        } else {
          ctx->Error("No const expression set for " + utils::number_to_position(i + 1) + " Generic Parameter " +
                         ctx->highlightError(generics.at(i)->getName().value) +
                         " and it doesn't have a default const expression associated with it",
                     generics.at(i)->getRange());
        }
      }
    }
  }
}

GenericToFill::GenericToFill(void* _data, GenericKind _kind, FileRange _range)
    : data(_data), kind(_kind), range(std::move(_range)) {}

GenericToFill* GenericToFill::GetPrerun(IR::PrerunValue* constVal, FileRange _range) {
  return new GenericToFill(constVal, GenericKind::prerunGeneric, std::move(_range));
}

GenericToFill* GenericToFill::GetType(IR::QatType* type, FileRange _range) {
  SHOW("GenericToFill of type is created")
  SHOW("Type is: " << type)
  SHOW("TypeKind is: " << (int)type->typeKind());
  return new GenericToFill(type, GenericKind::typedGeneric, std::move(_range));
}

bool GenericToFill::isType() const { return kind == GenericKind::typedGeneric; }

IR::QatType* GenericToFill::asType() const { return (IR::QatType*)data; }

bool GenericToFill::isPrerun() const { return kind == GenericKind::prerunGeneric; }

IR::PrerunValue* GenericToFill::asPrerun() const { return (IR::PrerunValue*)data; }

FileRange GenericToFill::getRange() const { return range; }

String GenericToFill::toString() const {
  if (isType()) {
    return asType()->toString();
  } else {
    return asPrerun()->getType()->toPrerunGenericString(asPrerun()).value();
  }
}

GenericParameter::GenericParameter(Identifier _name, GenericKind _kind, FileRange _range)
    : name(std::move(_name)), kind(_kind), range(std::move(_range)) {}

Identifier GenericParameter::getName() const { return name; }

FileRange GenericParameter::getRange() const { return range; }

bool GenericParameter::isSame(const String& cName) const { return name.value == cName; }

bool GenericParameter::isEqualTo(IR::Context* ctx, GenericToFill* fill) const {
  if (isTyped()) {
    if (fill->isType()) {
      return asTyped()->getType()->isSame(fill->asType());
    } else {
      if (fill->asPrerun()->getType()->isTyped()) {
        return asTyped()->getType()->isSame(fill->asPrerun()->getType()->asTyped()->getSubType());
      } else {
        return false;
      }
    }
  } else {
    if (fill->isType()) {
      if (asPrerun()->getType()->isTyped()) {
        return asPrerun()->getType()->asTyped()->isSame(fill->asType());
      } else {
        return false;
      }
    } else {
      return fill->asPrerun()->isEqualTo(ctx, asPrerun()->getExpression());
    }
  }
}

bool GenericParameter::isTyped() const { return kind == GenericKind::typedGeneric; }

TypedGeneric* GenericParameter::asTyped() const { return (TypedGeneric*)this; }

bool GenericParameter::isPrerun() const { return kind == GenericKind::prerunGeneric; }

PrerunGeneric* GenericParameter::asPrerun() const { return (PrerunGeneric*)this; }

String GenericParameter::toString() const {
  if (isPrerun()) {
    return asPrerun()->getExpression()->getType()->toPrerunGenericString(asPrerun()->getExpression()).value();
  } else if (isTyped()) {
    return asTyped()->getType()->toString();
  } else {
    // FIXME - Do something here
  }
}

TypedGeneric::TypedGeneric(Identifier _name, IR::QatType* _type, FileRange _range)
    : GenericParameter(std::move(_name), GenericKind::typedGeneric, std::move(_range)), type(_type) {}

TypedGeneric* TypedGeneric::get(Identifier name, IR::QatType* type, FileRange range) {
  return new TypedGeneric(std::move(name), type, std::move(range));
}

IR::QatType* TypedGeneric::getType() const { return type; }

Json TypedGeneric::toJson() const {
  return Json()._("name", name)._("genericKind", "typedGeneric")._("type", type->getID())._("range", range);
}

PrerunGeneric::PrerunGeneric(Identifier _name, IR::PrerunValue* _val, FileRange _range)
    : GenericParameter(std::move(_name), GenericKind::prerunGeneric, std::move(_range)), constant(_val) {}

PrerunGeneric* PrerunGeneric::get(Identifier name, IR::PrerunValue* val, FileRange range) {
  return new PrerunGeneric(std::move(name), val, std::move(range));
}

IR::PrerunValue* PrerunGeneric::getExpression() const { return constant; }

IR::QatType* PrerunGeneric::getType() const { return constant->getType(); };

Json PrerunGeneric::toJson() const {
  return Json()
      ._("name", name)
      ._("genericKind", "prerunGeneric")
      ._("type", constant->getType()->getID())
      ._("value", constant->getType()->canBePrerunGeneric()
                      ? constant->getType()->toPrerunGenericString(constant).value_or("")
                      : "")
      ._("range", range);
}

} // namespace qat::IR