#ifndef QAT_IR_GENERIC_VARIANT_HPP
#define QAT_IR_GENERIC_VARIANT_HPP

#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"
#include "./generics.hpp"
#include "./value.hpp"
#include "types/qat_type.hpp"

namespace qat::IR {

template <typename T> class GenericVariant {
private:
  T*                      entity;
  Vec<IR::GenericToFill*> genericTypes;

public:
  GenericVariant(T* _entity, Vec<IR::GenericToFill*> _types) : entity(_entity), genericTypes(std::move(_types)) {}

  useit bool check(std::function<void(const String&, const FileRange&)> errorFn, Vec<GenericToFill*> dest) const {
    if (genericTypes.size() != dest.size()) {
      return false;
    } else {
      for (usize i = 0; i < genericTypes.size(); i++) {
        auto* genTy = genericTypes.at(i);
        if (genTy->isType()) {
          if (dest.at(i)->isType()) {
            if (!genTy->asType()->isSame(dest.at(i)->asType())) {
              return false;
            }
          } else {
            auto* preVal = dest.at(i)->asPrerun();
            if (preVal->getType()->isTyped()) {
              if (!genTy->asType()->isSame(dest.at(i)->asPrerun()->getType()->asTyped()->getSubType())) {
                return false;
              }
            } else {
              return false;
            }
          }
        } else if (genTy->isPrerun()) {
          if (dest.at(i)->isPrerun()) {
            auto* genExp  = genTy->asPrerun();
            auto* destExp = dest.at(i)->asPrerun();
            if (genExp->getType()->isSame(destExp->getType())) {
              auto eqRes = genExp->getType()->equalityOf(genExp, destExp);
              if (eqRes.has_value()) {
                if (!eqRes.value()) {
                  return false;
                }
              } else {
                return false;
              }
            } else {
              return false;
            }
          } else {
            return false;
          }
        } else {
          errorFn("Invalid generic kind", genTy->getRange());
        }
      }
      return true;
    }
  }
  useit T* get() { return entity; }
};

} // namespace qat::IR

#endif