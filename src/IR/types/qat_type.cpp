#include "./qat_type.hpp"
#include "../../show.hpp"
#include "./array.hpp"
#include "./choice.hpp"
#include "./core_type.hpp"
#include "./definition.hpp"
#include "./float.hpp"
#include "./function.hpp"
#include "./future.hpp"
#include "./integer.hpp"
#include "./maybe.hpp"
#include "./mix.hpp"
#include "./pointer.hpp"
#include "./reference.hpp"
#include "./region.hpp"
#include "./string_slice.hpp"
#include "./tuple.hpp"
#include "./type_kind.hpp"
#include "./unsigned.hpp"
#include "c_type.hpp"
#include "opaque.hpp"
#include "result.hpp"

namespace qat::IR {

QatType::QatType() { types.push_back(this); }

Vec<QatType*> QatType::types = {};

Vec<Region*> QatType::allRegions() {
  Vec<Region*> result;
  for (auto* typ : types) {
    if (typ->typeKind() == TypeKind::region) {
      result.push_back(typ->asRegion());
    }
  }
  return result;
}

void QatType::clearAll() {
  for (auto* typ : types) {
    delete typ;
  }
}

bool QatType::hasNoValueSemantics() const { return false; }

String QatType::getNameForLinking() const { return linkingName; }

bool QatType::canBePrerunGeneric() const { return false; }

Maybe<String> QatType::toPrerunGenericString(IR::PrerunValue* val) const { return None; }

bool QatType::isTypeSized() const { return false; }

Maybe<bool> QatType::equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const { return false; }

bool QatType::checkTypeExists(const String& name) {
  for (const auto& typ : types) {
    if (typ->typeKind() == TypeKind::mixType) {
      if (((MixType*)typ)->getFullName() == name) {
        return true;
      }
    } else if (typ->typeKind() == TypeKind::core) {
      if (((CoreType*)typ)->getFullName() == name) {
        return true;
      }
    }
  }
  return false;
}

bool QatType::isCompatible(QatType* other) {
  if (isPointer() && other->isPointer()) {
    if ((asPointer()->getSubType()->isSame(other->asPointer()->getSubType())) &&
        asPointer()->getOwner().isAnonymous() && (asPointer()->isMulti() == other->asPointer()->isMulti())) {
      return true;
    } else {
      return isSame(other);
    }
  } else if (isUnsignedInteger() && other->isUnsignedInteger()) {
    if (asUnsignedInteger()->getBitwidth() == other->asUnsignedInteger()->getBitwidth()) {
      return true;
    } else {
      return isSame(other);
    }
  } else {
    return isSame(other);
  }
}

bool QatType::isSame(QatType* other) {
  if (typeKind() != other->typeKind()) {
    if (typeKind() == TypeKind::definition) {
      return ((DefinitionType*)this)->getSubType()->isSame(other);
    } else if (other->typeKind() == TypeKind::definition) {
      return ((DefinitionType*)other)->getSubType()->isSame(this);
    } else if (typeKind() == TypeKind::opaque) {
      if (((OpaqueType*)this)->hasSubType()) {
        return ((OpaqueType*)this)->getSubType()->isSame(other);
      } else {
        return false;
      }
    } else if (other->typeKind() == TypeKind::opaque) {
      if (((OpaqueType*)other)->hasSubType()) {
        return (((OpaqueType*)other)->getSubType()->isSame(this));
      } else {
        return false;
      }
    }
    return false;
  } else {
    switch (typeKind()) {
      case TypeKind ::definition: {
        return ((DefinitionType*)this)->getSubType()->isSame(((DefinitionType*)other)->getSubType());
      }
      case TypeKind::opaque: {
        auto* thisVal  = (OpaqueType*)this;
        auto* otherVal = (OpaqueType*)other;
        if (thisVal->hasSubType() && otherVal->hasSubType()) {
          return thisVal->getSubType()->isSame(otherVal->getSubType());
        } else {
          return thisVal->getID() == otherVal->getID();
        }
      }
      case TypeKind::pointer: {
        return (((PointerType*)this)->isSubtypeVariable() == ((PointerType*)other)->isSubtypeVariable()) &&
               (((PointerType*)this)->isNullable() == ((PointerType*)other)->isNullable()) &&
               (((PointerType*)this)->getSubType()->isSame(((PointerType*)other)->getSubType())) &&
               (((PointerType*)this)->getOwner().isSame(((PointerType*)other)->getOwner()));
      }
      case TypeKind::reference: {
        return (((ReferenceType*)this)->isSubtypeVariable() == ((ReferenceType*)other)->isSubtypeVariable()) &&
               (((ReferenceType*)this)->getSubType()->isSame(((ReferenceType*)other)->getSubType()));
      }
      case TypeKind::future: {
        auto* thisVal  = (FutureType*)this;
        auto* otherVal = (FutureType*)other;
        return thisVal->getSubType()->isSame(otherVal->getSubType()) &&
               (thisVal->isTypePacked() == otherVal->isTypePacked());
      }
      case TypeKind::maybe: {
        auto* thisVal  = (MaybeType*)this;
        auto* otherVal = (MaybeType*)this;
        return thisVal->getSubType()->isSame(otherVal->getSubType()) &&
               (thisVal->isTypePacked() == otherVal->isTypePacked());
      }
      case TypeKind::unsignedInteger: {
        return (((UnsignedType*)this)->getBitwidth() == ((UnsignedType*)other)->getBitwidth()) &&
               (((UnsignedType*)this)->isBoolean() == ((UnsignedType*)other)->isBoolean());
      }
      case TypeKind::integer: {
        return (((IntegerType*)this)->getBitwidth() == ((IntegerType*)other)->getBitwidth());
      }
      case TypeKind::Float: {
        return (((FloatType*)this)->getKind() == ((FloatType*)other)->getKind());
      }
      case TypeKind::cType: {
        auto* thisVal  = (CType*)this;
        auto* otherVal = (CType*)other;
        return thisVal->get_c_type_kind() == otherVal->get_c_type_kind() &&
               (thisVal->isCPointer() ? (thisVal->getSubType()->isSame(otherVal->getSubType())) : true);
      }
      case TypeKind::stringSlice: {
        auto* thisVal  = (StringSliceType*)this;
        auto* otherVal = (StringSliceType*)other;
        return thisVal->isPacked() == otherVal->isPacked();
      }
      case TypeKind::Void: {
        return true;
      }
      case TypeKind::typed: {
        return ((TypedType*)this)->getSubType()->isSame(((TypedType*)other)->getSubType());
      }
      case TypeKind::array: {
        auto* thisVal  = (ArrayType*)this;
        auto* otherVal = (ArrayType*)other;
        if (thisVal->getLength() == otherVal->getLength()) {
          return thisVal->getElementType()->isSame(otherVal->getElementType());
        } else {
          return false;
        }
      }
      case TypeKind::tuple: {
        auto* thisVal  = (TupleType*)this;
        auto* otherVal = (TupleType*)other;
        if ((thisVal->isPackedTuple() == otherVal->isPackedTuple()) &&
            thisVal->getSubTypeCount() == otherVal->getSubTypeCount()) {
          for (usize i = 0; i < thisVal->getSubTypeCount(); i++) {
            if (!(thisVal->getSubtypeAt(i)->isSame(otherVal->getSubtypeAt(i)))) {
              return false;
            }
          }
          return true;
        } else {
          return false;
        }
      }
      case TypeKind::core: {
        auto* thisVal  = (CoreType*)this;
        auto* otherVal = (CoreType*)other;
        return (thisVal->getID() == otherVal->getID());
      }
      case TypeKind::region: {
        auto* thisVal  = (Region*)this;
        auto* otherVal = (Region*)this;
        return (thisVal->getID() == otherVal->getID());
      }
      case TypeKind::choice: {
        auto* thisVal  = (ChoiceType*)this;
        auto* otherVal = (ChoiceType*)other;
        return (thisVal->getID() == otherVal->getID());
      }
      case TypeKind::mixType: {
        auto* thisVal  = (MixType*)this;
        auto* otherVal = (MixType*)other;
        return (thisVal->getID() == otherVal->getID());
      }
      case TypeKind::result: {
        auto* thisVal  = (ResultType*)this;
        auto* otherVal = (ResultType*)other;
        return (thisVal->isPacked == otherVal->isPacked) && thisVal->getValidType()->isSame(otherVal->getValidType()) &&
               thisVal->getErrorType()->isSame(otherVal->getErrorType());
      }
      case TypeKind::function: {
        auto* thisVal  = (FunctionType*)this;
        auto* otherVal = (FunctionType*)other;
        if (thisVal->getArgumentCount() == otherVal->getArgumentCount()) {
          if (thisVal->getReturnType()->isSame(otherVal->getReturnType())) {
            for (usize i = 0; i < thisVal->getArgumentCount(); i++) {
              auto* thisArg  = thisVal->getArgumentTypeAt(i);
              auto* otherArg = otherVal->getArgumentTypeAt(i);
              if (thisArg->isVariable() != otherArg->isVariable()) {
                return false;
              } else {
                if (!thisArg->getType()->isSame(otherArg->getType())) {
                  return false;
                }
              }
            }
            return true;
          } else {
            return false;
          }
        } else {
          return false;
        }
      }
    }
  }
}

bool QatType::isExpanded() const { return false; }

ExpandedType* QatType::asExpanded() const {
  return (typeKind() == TypeKind::definition)
             ? asDefinition()->getSubType()->asExpanded()
             : (isOpaque() ? asOpaque()->getSubType()->asExpanded() : (ExpandedType*)this);
}

bool QatType::isTyped() const {
  return (typeKind() == TypeKind::typed) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isOpaque());
}

TypedType* QatType::asTyped() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asTyped() : (TypedType*)this;
}

bool QatType::isOpaque() const {
  return (typeKind() == TypeKind::opaque) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isOpaque());
}

OpaqueType* QatType::asOpaque() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asOpaque() : (OpaqueType*)this;
}

bool QatType::isTriviallyCopyable() const { return false; }
bool QatType::isTriviallyMovable() const { return false; }

bool QatType::isCopyConstructible() const { return false; }
void QatType::copyConstructValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {}

bool QatType::isCopyAssignable() const { return false; }
void QatType::copyAssignValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {}

bool QatType::isMoveConstructible() const { return false; }
void QatType::moveConstructValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {}

bool QatType::isMoveAssignable() const { return false; }
void QatType::moveAssignValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {}

bool QatType::isDestructible() const { return false; }
void QatType::destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {}

bool QatType::isDefinition() const {
  return (typeKind() == TypeKind::definition) ||
         ((typeKind() == TypeKind::opaque) && asOpaque()->hasSubType() && asOpaque()->getSubType()->isDefinition());
}

DefinitionType* QatType::asDefinition() const { return (DefinitionType*)this; }

bool QatType::isInteger() const {
  return (typeKind() == TypeKind::integer) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isInteger()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isInteger());
}

IntegerType* QatType::asInteger() const {
  return (typeKind() == TypeKind::definition)
             ? ((DefinitionType*)this)->getSubType()->asInteger()
             : (isOpaque() ? asOpaque()->getSubType()->asInteger() : (IntegerType*)this);
}

bool QatType::isUnsignedInteger() const {
  return (typeKind() == TypeKind::unsignedInteger) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isUnsignedInteger()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isUnsignedInteger());
}

UnsignedType* QatType::asUnsignedInteger() const {
  return (typeKind() == TypeKind::definition)
             ? ((DefinitionType*)this)->getSubType()->asUnsignedInteger()
             : (isOpaque() ? asOpaque()->getSubType()->asUnsignedInteger() : (UnsignedType*)this);
}

bool QatType::isBool() const { return isUnsignedInteger() && asUnsignedInteger()->isBoolean(); }

UnsignedType* QatType::asBool() const { return asUnsignedInteger(); }

bool QatType::isFloat() const {
  return (typeKind() == TypeKind::Float) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isFloat()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isFloat());
}

FloatType* QatType::asFloat() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asFloat()
                                              : (isOpaque() ? asOpaque()->getSubType()->asFloat() : (FloatType*)this);
}

bool QatType::isReference() const {
  return (typeKind() == TypeKind::reference) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isReference()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isReference());
}

ReferenceType* QatType::asReference() const {
  return (typeKind() == TypeKind::definition)
             ? ((DefinitionType*)this)->getSubType()->asReference()
             : (isOpaque() ? asOpaque()->getSubType()->asReference() : (ReferenceType*)this);
}

bool QatType::isPointer() const {
  return (typeKind() == TypeKind::pointer) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isPointer()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isPointer());
}

PointerType* QatType::asPointer() const {
  return (typeKind() == TypeKind::definition)
             ? ((DefinitionType*)this)->getSubType()->asPointer()
             : (isOpaque() ? asOpaque()->getSubType()->asPointer() : (PointerType*)this);
}

bool QatType::isArray() const {
  return (typeKind() == TypeKind::array) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isArray()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isArray());
}

ArrayType* QatType::asArray() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asArray()
                                              : (isOpaque() ? asOpaque()->getSubType()->asArray() : (ArrayType*)this);
}

bool QatType::isTuple() const {
  return (typeKind() == TypeKind::tuple) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isTuple()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isTuple());
}

TupleType* QatType::asTuple() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asTuple()
                                              : (isOpaque() ? asOpaque()->getSubType()->asTuple() : (TupleType*)this);
}

bool QatType::isFunction() const {
  return (typeKind() == TypeKind::function) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isFunction()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isFunction());
}

FunctionType* QatType::asFunction() const {
  return (typeKind() == TypeKind::definition)
             ? ((DefinitionType*)this)->getSubType()->asFunction()
             : (isOpaque() ? asOpaque()->getSubType()->asFunction() : (FunctionType*)this);
}

bool QatType::isCoreType() const {
  return (typeKind() == TypeKind::core) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isCoreType()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isCoreType());
}

CoreType* QatType::asCore() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asCore()
                                              : (isOpaque() ? asOpaque()->getSubType()->asCore() : (CoreType*)this);
}

bool QatType::isMix() const {
  return (typeKind() == TypeKind::mixType) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isMix()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isMix());
}

MixType* QatType::asMix() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asMix()
                                              : (isOpaque() ? asOpaque()->getSubType()->asMix() : (MixType*)this);
}

bool QatType::isChoice() const {
  return ((typeKind() == TypeKind::choice) ||
          (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isChoice()) ||
          (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isChoice()));
}

ChoiceType* QatType::asChoice() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asChoice()
                                              : (isOpaque() ? asOpaque()->getSubType()->asChoice() : (ChoiceType*)this);
}

bool QatType::isVoid() const {
  return (typeKind() == TypeKind::Void) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isVoid()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isVoid());
}

bool QatType::isStringSlice() const {
  return (typeKind() == TypeKind::stringSlice) ||
         (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isStringSlice()) ||
         (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isStringSlice());
}

StringSliceType* QatType::asStringSlice() const {
  return (typeKind() == TypeKind::definition)
             ? ((DefinitionType*)this)->getSubType()->asStringSlice()
             : (isOpaque() ? asOpaque()->getSubType()->asStringSlice() : (StringSliceType*)this);
}

bool QatType::isCType() const {
  return ((typeKind() == TypeKind::cType) ||
          (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isCType()) ||
          (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isCType()));
}

CType* QatType::asCType() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asCType()
                                              : (isOpaque() ? asOpaque()->getSubType()->asCType() : (CType*)this);
}

bool QatType::isFuture() const {
  return ((typeKind() == TypeKind::future) ||
          (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isFuture()) ||
          (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isFuture()));
}

FutureType* QatType::asFuture() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asFuture()
                                              : (isOpaque() ? asOpaque()->getSubType()->asFuture() : (FutureType*)this);
}

bool QatType::isMaybe() const {
  return ((typeKind() == TypeKind::maybe) ||
          (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isMaybe()) ||
          (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isMaybe()));
}

MaybeType* QatType::asMaybe() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asMaybe()
                                              : (isOpaque() ? asOpaque()->getSubType()->asMaybe() : (MaybeType*)this);
}

bool QatType::isRegion() const {
  return ((typeKind() == TypeKind::region) ||
          (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isRegion()) ||
          (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isRegion()));
}

Region* QatType::asRegion() const {
  return (typeKind() == TypeKind::definition) ? ((DefinitionType*)this)->getSubType()->asRegion()
                                              : (isOpaque() ? asOpaque()->getSubType()->asRegion() : (Region*)this);
}

bool QatType::isResult() const {
  return ((typeKind() == TypeKind::result) ||
          (isOpaque() && asOpaque()->hasSubType() && asOpaque()->getSubType()->isResult()) ||
          (typeKind() == TypeKind::definition && asDefinition()->getSubType()->isResult()));
}

ResultType* QatType::asResult() const {
  return ((typeKind() == TypeKind::definition)
              ? ((DefinitionType*)this)->getSubType()->asResult()
              : (isOpaque() ? asOpaque()->getSubType()->asResult() : (ResultType*)this));
}

} // namespace qat::IR
