#include "./local_value.hpp"
#include "../AST/types/qat_type.hpp"

namespace qat {
namespace IR {

LocalValue::LocalValue(std::string _name, QatType *_type, bool is_variable,
                       Value *initial, Block *bb, Context *ctx)
    : name(_name), loads(0), stores(0), refers(0),
      Value(_type, is_variable, Kind::assignable) {}

bool LocalValue::isRemovable() const {
  return ((((getType()->typeKind() != IR::TypeKind::reference) &&
            (getType()->typeKind() != IR::TypeKind::pointer))
               ? ((loads == 0) && (refers == 0))
               : false) ||
          ((loads == 0) && (stores == 0) && (refers == 0)));
}

} // namespace IR
} // namespace qat