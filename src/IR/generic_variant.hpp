#ifndef QAT_IR_GENERIC_VARIANT_HPP
#define QAT_IR_GENERIC_VARIANT_HPP

#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"
#include "types/qat_type.hpp"

namespace qat::IR {

template <typename T> class GenericVariant {
private:
  T*                entity;
  Vec<IR::QatType*> types;

public:
  GenericVariant(T* _entity, Vec<IR::QatType*> _types) : entity(_entity), types(std::move(_types)) {}

  useit bool check(Vec<IR::QatType*> _types) const {
    if (types.size() != _types.size()) {
      return false;
    } else {
      bool result = true;
      for (usize i = 0; i < types.size(); i++) {
        if (!types.at(i)->isSame(_types.at(i))) {
          result = false;
          break;
        }
      }
      return result;
    }
  }
  useit T* get() { return entity; }
};

} // namespace qat::IR

#endif