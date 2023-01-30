#include "./generics.hpp"
#include "../ast/types/const_generic.hpp"
#include "../ast/types/generic_abstract.hpp"
#include "../ast/types/typed_generic.hpp"
#include "../utils/number_to_position.hpp"
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
          if (types.at(i)->asConst()->getType()->isTyped()) {
            generics.at(i)->asTyped()->setType(types.at(i)->asConst()->getType()->asTyped()->getSubType());
          } else {
            ctx->Error(
                utils::numberToPosition(i + 1) + " Generic Parameter " +
                    ctx->highlightError(generics.at(i)->getName().value) +
                    " expects a type or a constant expression that gives a type, but a normal constant expression was provided",
                fileRange);
          }
        }
      } else {
        if (generics.at(i)->isSet()) {
          types.push_back(IR::GenericToFill::get(generics.at(i)->asTyped()->getType(), generics.at(i)->getRange()));
        } else {
          ctx->Error("No type set for " + utils::numberToPosition(i + 1) + " Generic Parameter " +
                         ctx->highlightError(generics.at(i)->getName().value) +
                         " and it doesn't have a default type associated with it",
                     generics.at(i)->getRange());
        }
      }
    } else {
      if (types.size() > i) {
        if (types.at(i)->isConst() || generics.at(i)->asConst()->getType()->isTyped()) {
          if (types.at(i)->isConst()) {
            generics.at(i)->asConst()->setExpression(types.at(i)->asConst());
          } else {
            generics.at(i)->asConst()->setExpression(new IR::ConstantValue(IR::TypedType::get(types.at(i)->asType())));
          }
        } else {
          ctx->Error(utils::numberToPosition(i + 1) + " Generic Parameter " +
                         ctx->highlightError(generics.at(i)->getName().value) +
                         " expects a constant expression, not a type",
                     fileRange);
        }
      } else {
        if (generics.at(i)->isSet()) {
          types.push_back(IR::GenericToFill::get(generics.at(i)->asConst()->getConstant(), generics.at(i)->getRange()));
        } else {
          ctx->Error("No const expression set for " + utils::numberToPosition(i + 1) + " Generic Parameter " +
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

GenericToFill* GenericToFill::get(IR::ConstantValue* constVal, FileRange _range) {
  return new GenericToFill(constVal, GenericKind::constGeneric, std::move(_range));
}

GenericToFill* GenericToFill::get(IR::QatType* type, FileRange _range) {
  SHOW("GenericToFill of type is created")
  SHOW("Type is: " << type)
  SHOW("TypeKind is: " << (int)type->typeKind());
  return new GenericToFill(type, GenericKind::typedGeneric, std::move(_range));
}

bool GenericToFill::isType() const { return kind == GenericKind::typedGeneric; }

IR::QatType* GenericToFill::asType() const { return (IR::QatType*)data; }

bool GenericToFill::isConst() const { return kind == GenericKind::constGeneric; }

IR::ConstantValue* GenericToFill::asConst() const { return (IR::ConstantValue*)data; }

FileRange GenericToFill::getRange() const { return range; }

String GenericToFill::toString() const {
  if (isType()) {
    SHOW("GenericToFill is type")
    return asType()->toString();
  } else {
    return asConst()->getType()->toConstGenericString(asConst()).value();
  }
}

GenericType::GenericType(Identifier _name, GenericKind _kind, FileRange _range)
    : name(std::move(_name)), kind(_kind), range(std::move(_range)) {}

Identifier GenericType::getName() const { return name; }

FileRange GenericType::getRange() const { return range; }

bool GenericType::isSame(const String& cName) const { return name.value == cName; }

bool GenericType::isEqualTo(GenericToFill* fill) const {
  if (isTyped()) {
    if (fill->isType()) {
      return asTyped()->getType()->isSame(fill->asType());
    } else {
      if (fill->asConst()->getType()->isTyped()) {
        return asTyped()->getType()->isSame(fill->asConst()->getType()->asTyped()->getSubType());
      } else {
        return false;
      }
    }
  } else {
    if (fill->isType()) {
      if (asConst()->getType()->isTyped()) {
        return asConst()->getType()->asTyped()->isSame(fill->asType());
      } else {
        return false;
      }
    } else {
      return fill->asConst()->isEqualTo(asConst()->getExpression());
    }
  }
}

bool GenericType::isTyped() const { return kind == GenericKind::typedGeneric; }

TypedGeneric* GenericType::asTyped() const { return (TypedGeneric*)this; }

bool GenericType::isConst() const { return kind == GenericKind::constGeneric; }

ConstGeneric* GenericType::asConst() const { return (ConstGeneric*)this; }

TypedGeneric::TypedGeneric(Identifier _name, IR::QatType* _type, FileRange _range)
    : GenericType(std::move(_name), GenericKind::typedGeneric, std::move(_range)), type(_type) {}

TypedGeneric* TypedGeneric::get(Identifier name, IR::QatType* type, FileRange range) {
  return new TypedGeneric(std::move(name), type, std::move(range));
}

IR::QatType* TypedGeneric::getType() const { return type; }

ConstGeneric::ConstGeneric(Identifier _name, IR::ConstantValue* _val, FileRange _range)
    : GenericType(std::move(_name), GenericKind::constGeneric, std::move(_range)), constant(_val) {}

ConstGeneric* ConstGeneric::get(Identifier name, IR::ConstantValue* val, FileRange range) {
  return new ConstGeneric(std::move(name), val, std::move(range));
}

IR::ConstantValue* ConstGeneric::getExpression() const { return constant; }

IR::QatType* ConstGeneric::getType() const { return constant->getType(); };

} // namespace qat::IR