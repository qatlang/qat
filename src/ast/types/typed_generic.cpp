#include "./typed_generic.hpp"
#include "generic_abstract.hpp"

namespace qat::ast {

TypedGeneric::TypedGeneric(usize _index, Identifier _name, Maybe<ast::QatType*> _defaultTy, FileRange _fileRange)
    : GenericAbstractType(_index, std::move(_name), GenericKind::typedGeneric, std::move(_fileRange)),
      defaultTypeAST(_defaultTy){SHOW("Typed Generic") SHOW("   name: " << name.value)
                                     SHOW("   hasDefault: " << defaultTypeAST.has_value())}

          TypedGeneric
          * TypedGeneric::get(usize _index, Identifier _name, Maybe<ast::QatType*> _defaultTy, FileRange _fileRange) {
  return new TypedGeneric(_index, std::move(_name), _defaultTy, std::move(_fileRange));
}

bool TypedGeneric::hasDefault() const { return defaultTypeAST.has_value(); }

IR::QatType* TypedGeneric::getDefault() const { return defaultType; }

void TypedGeneric::setType(IR::QatType* typ) const { typeValue = typ; }

bool TypedGeneric::isSet() const { return (typeValue != nullptr) || (defaultType != nullptr); }

void TypedGeneric::unset() const { typeValue = nullptr; }

void TypedGeneric::emit(IR::Context* ctx) const {
  SHOW("Emitting typed generic " << name.value << " and hasDefault: " << hasDefault())
  if (hasDefault()) {
    defaultType = defaultTypeAST.value()->emit(ctx);
  }
}

IR::TypedGeneric* TypedGeneric::toIR() const { return IR::TypedGeneric::get(name, getType(), range); }

IR::QatType* TypedGeneric::getType() const { return typeValue ? typeValue : defaultType; }

Json TypedGeneric::toJson() const {
  return Json()
      ._("genericKind", "typedGeneric")
      ._("index", index)
      ._("name", name)
      ._("hasDefault", defaultTypeAST.has_value())
      ._("defaultType", defaultTypeAST.has_value() ? defaultTypeAST.value()->toJson() : Json())
      ._("range", range);
}

TypedGeneric::~TypedGeneric() {}

} // namespace qat::ast