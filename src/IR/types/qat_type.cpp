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

std::vector<QatType *> QatType::types = {};

bool QatType::checkTypeExists(const std::string name) {
  for (auto typ : types) {
    if (typ->typeKind() == TypeKind::sumType) {
      if (((SumType *)typ)->getFullName() == name) {
        return true;
      }
    } else if (typ->typeKind() == TypeKind::core) {
      if (((CoreType *)typ)->getFullName() == name) {
        return true;
      }
    }
  }
}

bool QatType::isSame(QatType *other) const {
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
      auto thisVal = (ArrayType *)this;
      auto otherVal = (ArrayType *)other;
      if (thisVal->getLength() == otherVal->getLength()) {
        return thisVal->getElementType()->isSame(otherVal->getElementType());
      } else {
        return false;
      }
    }
    case TypeKind::tuple: {
      auto thisVal = (TupleType *)this;
      auto otherVal = (TupleType *)other;
      if (thisVal->getSubTypeCount() == otherVal->getSubTypeCount()) {
        for (std::size_t i = 0; i < thisVal->getSubTypeCount(); i++) {
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
      auto thisVal = (CoreType *)this;
      auto otherVal = (CoreType *)other;
      return (thisVal->getFullName() == otherVal->getFullName());
    }
    case TypeKind::sumType: {
      auto thisVal = (SumType *)this;
      auto otherVal = (SumType *)other;
      if (thisVal->getFullName() == otherVal->getFullName()) {
        if (thisVal->getSubtypeCount() == otherVal->getSubtypeCount()) {
          for (std::size_t i = 0; i < thisVal->getSubtypeCount(); i++) {
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
      auto thisVal = (FunctionType *)this;
      auto otherVal = (FunctionType *)other;
      if (thisVal->getArgumentCount() == otherVal->getArgumentCount()) {
        if (thisVal->getReturnType()->isSame(otherVal->getReturnType())) {
          for (std::size_t i = 0; i < thisVal->getArgumentCount(); i++) {
            auto thisArg = thisVal->getArgumentTypeAt(i);
            auto otherArg = otherVal->getArgumentTypeAt(i);
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

} // namespace qat::IR