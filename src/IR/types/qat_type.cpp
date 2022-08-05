#include "./qat_type.hpp"
#include "./array.hpp"
#include "./core_type.hpp"
#include "./float.hpp"
#include "./function.hpp"
#include "./integer.hpp"
#include "./pointer.hpp"
#include "./reference.hpp"
#include "./sum.hpp"
#include "./tuple.hpp"
#include "./type_kind.hpp"
#include "./unsigned.hpp"

namespace qat::IR {

QatType::QatType() { types.push_back(this); }

Vec<QatType *> QatType::types = {};

bool QatType::checkTypeExists(const String &name) {
  return std::ranges::any_of(types.begin(), types.end(), [&](QatType *typ) {
    if (typ->typeKind() == TypeKind::sumType) {
      if (((SumType *)typ)->getFullName() == name) {
        return true;
      }
    } else if (typ->typeKind() == TypeKind::core) {
      if (((CoreType *)typ)->getFullName() == name) {
        return true;
      }
    }
    return false;
  });
}

bool QatType::isSame(QatType *other) const { // NOLINT(misc-no-recursion)
  if (typeKind() != other->typeKind()) {
    return false;
  } else {
    switch (typeKind()) {
    case TypeKind::pointer: {
      return (((PointerType *)this)
                  ->getSubType()
                  ->isSame(((PointerType *)other)->getSubType()));
    }
    case TypeKind::reference: {
      return (((ReferenceType *)this)
                  ->getSubType()
                  ->isSame(((ReferenceType *)other)->getSubType()));
    }
    case TypeKind::unsignedInteger: {
      return (((UnsignedType *)this)->getBitwidth() ==
              ((UnsignedType *)other)->getBitwidth());
    }
    case TypeKind::integer: {
      return (((IntegerType *)this)->getBitwidth() ==
              ((IntegerType *)other)->getBitwidth());
    }
    case TypeKind::Float: {
      return (((FloatType *)this)->getKind() ==
              ((FloatType *)other)->getKind());
    }
    case TypeKind::array: {
      auto *thisVal  = (ArrayType *)this;
      auto *otherVal = (ArrayType *)other;
      if (thisVal->getLength() == otherVal->getLength()) {
        return thisVal->getElementType()->isSame(otherVal->getElementType());
      } else {
        return false;
      }
    }
    case TypeKind::tuple: {
      auto *thisVal  = (TupleType *)this;
      auto *otherVal = (TupleType *)other;
      if (thisVal->getSubTypeCount() == otherVal->getSubTypeCount()) {
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
      auto *thisVal  = (CoreType *)this;
      auto *otherVal = (CoreType *)other;
      return (thisVal->getFullName() == otherVal->getFullName());
    }
    case TypeKind::sumType: {
      auto *thisVal  = (SumType *)this;
      auto *otherVal = (SumType *)other;
      if (thisVal->getFullName() == otherVal->getFullName()) {
        if (thisVal->getSubtypeCount() == otherVal->getSubtypeCount()) {
          for (usize i = 0; i < thisVal->getSubtypeCount(); i++) {
            if (!thisVal->getSubtypeAt(i)->isSame(otherVal->getSubtypeAt(i))) {
              return false;
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
    case TypeKind::function: {
      auto *thisVal  = (FunctionType *)this;
      auto *otherVal = (FunctionType *)other;
      if (thisVal->getArgumentCount() == otherVal->getArgumentCount()) {
        if (thisVal->getReturnType()->isSame(otherVal->getReturnType())) {
          for (usize i = 0; i < thisVal->getArgumentCount(); i++) {
            auto *thisArg  = thisVal->getArgumentTypeAt(i);
            auto *otherArg = otherVal->getArgumentTypeAt(i);
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

bool QatType::isInteger() const { return typeKind() == TypeKind::integer; }

IntegerType *QatType::asInteger() const { return (IntegerType *)this; }

bool QatType::isUnsignedInteger() const {
  return typeKind() == TypeKind::unsignedInteger;
}

UnsignedType *QatType::asUnsignedInteger() const {
  return (UnsignedType *)this;
}

bool QatType::isFloat() const { return typeKind() == TypeKind::Float; }

FloatType *QatType::asFloat() const { return (FloatType *)this; }

bool QatType::isReference() const { return typeKind() == TypeKind::reference; }

ReferenceType *QatType::asReference() const { return (ReferenceType *)this; }

bool QatType::isPointer() const { return typeKind() == TypeKind::pointer; }

PointerType *QatType::asPointer() const { return (PointerType *)this; }

bool QatType::isArray() const { return typeKind() == TypeKind::array; }

ArrayType *QatType::asArray() const { return (ArrayType *)this; }

bool QatType::isTuple() const { return typeKind() == TypeKind::tuple; }

TupleType *QatType::asTuple() const { return (TupleType *)this; }

bool QatType::isFunction() const { return typeKind() == TypeKind::function; }

FunctionType *QatType::asFunction() const { return (FunctionType *)this; }

bool QatType::isTemplate() const {
  return ((typeKind() == TypeKind::templateCoreType) ||
          (typeKind() == TypeKind::templatePointer) ||
          (typeKind() == TypeKind::templateSumType) ||
          (typeKind() == TypeKind::templateTuple));
}

} // namespace qat::IR