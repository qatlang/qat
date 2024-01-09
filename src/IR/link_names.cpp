#include "./link_names.hpp"
#include "./qat_module.hpp"

namespace qat {

LinkNames::LinkNames() : units(), foreignID(), parentMod(nullptr) {}

LinkNames::LinkNames(Vec<LinkNameUnit> _units, Maybe<String> _foreignID, IR::QatModule* _mod)
    : units(_units), foreignID(_foreignID), parentMod(_mod) {}

void LinkNames::addUnit(LinkNameUnit unit, Maybe<String> entityForeignID) {
  units.push_back(unit);
  if (entityForeignID) {
    foreignID = entityForeignID;
  }
}

LinkNames LinkNames::newWith(LinkNameUnit unit, Maybe<String> entityForeignID) {
  auto result = *this;
  result.addUnit(unit, entityForeignID);
  return result;
}

String LinkNames::toName() const {
  auto isForeign = [&](String const& id) {
    if (foreignID.has_value()) {
      return (foreignID.value() == id);
    } else {
      return parentMod && parentMod->isInForeignModuleOfType(id);
    }
  };
  if (isForeign("C")) {
    return units.back().getUsableName();
  } else if (isForeign("C++")) {
    String result("");
    // FIXME - Implement C++ name mangling
    return result;
  } else {
    String result = ((!units.empty()) && (units.front().unitType == LinkUnitType::name ||
                                          units.front().unitType == LinkUnitType::genericTypeValue ||
                                          units.front().unitType == LinkUnitType::genericPrerunValue))
                        ? ""
                        : "qat'";
    for (usize i = 0; i < units.size(); i++) {
      auto& unit = units.at(i);
      if (i != 0 && unit.unitType != LinkUnitType::genericList) {
        result += ":";
      }
      switch (unit.unitType) {
        case LinkUnitType::box: {
          result += "box_";
          result += unit.getUsableName();
          break;
        }
        case LinkUnitType::lib: {
          result += "lib_";
          result += unit.getUsableName();
          break;
        }
        case LinkUnitType::type: {
          result += "type_";
          result += unit.getUsableName();
          break;
        }
        case LinkUnitType::choice: {
          result += "type_choice_";
          result += unit.getUsableName();
          break;
        }
        case LinkUnitType::mix: {
          result += "type_mix_";
          result += unit.getUsableName();
          break;
        }
        case LinkUnitType::staticField: {
          result += "static_";
          result += unit.getUsableName();
          break;
        }
        case LinkUnitType::region: {
          result += "region_";
          result += unit.getUsableName();
          break;
        }
        case LinkUnitType::doType: {
          result += "do_type:[" + unit.subNames.front().toName() + "]";
          break;
        }
        case LinkUnitType::skill: {
          result += "skill_";
          result += unit.getUsableName();
          break;
        }
        case LinkUnitType::doSkill: {
          result += "do_skill:[" + unit.getUsableName() + "]:for_type:[" + unit.subNames.front().toName() + "]";
          break;
        }
        case LinkUnitType::function: {
          result += "fn_" + unit.getUsableName();
          break;
        }
        case LinkUnitType::staticFunction: {
          result += "staticfn_" + unit.getUsableName();
          break;
        }
        case LinkUnitType::variationMethod: {
          result += "variation_" + unit.getUsableName();
          break;
        }
        case LinkUnitType::method: {
          result += "method_" + unit.getUsableName();
          break;
        }
        case LinkUnitType::defaultConstructor: {
          result += "constructor_default";
          break;
        }
        case LinkUnitType::fromConvertor: {
          result += "convertor_from:[" + unit.getUsableName() + "]";
          break;
        }
        case LinkUnitType::toConvertor: {
          result += "convertor_to:[" + unit.getUsableName() + "]";
          break;
        }
        case LinkUnitType::constructor: {
          result += "constructor_from:[";
          for (usize j = 0; j < unit.subNames.size(); j++) {
            result += unit.subNames.at(j).toName();
            if (j != (unit.subNames.size() - 1)) {
              result += ",";
            }
          }
          result += "]";
          break;
        }
        case LinkUnitType::copyConstructor: {
          result += "constructor_copy";
          break;
        }
        case LinkUnitType::copyAssignment: {
          result += "operator_copy";
          break;
        }
        case LinkUnitType::moveConstructor: {
          result += "constructor_move";
          break;
        }
        case LinkUnitType::moveAssignment: {
          result += "operator_move";
          break;
        }
        case LinkUnitType::normalBinaryOperator: {
          result += "operator_binary(" + unit.getUsableName() + "):[" + unit.subNames.front().toName() + "]";
          break;
        }
        case LinkUnitType::variationBinaryOperator: {
          result += "operator_binary_variation(" + unit.getUsableName() + "):[" + unit.subNames.front().toName() + "]";
          break;
        }
        case LinkUnitType::unaryOperator: {
          result += "operator_unary(" + unit.getUsableName() + ")";
        }
        case LinkUnitType::destructor: {
          result += "destructor";
          break;
        }
        case LinkUnitType::global: {
          result += "global_" + unit.getUsableName();
          break;
        }
        case LinkUnitType::genericList: {
          result += ":[";
          for (usize j = 0; j < unit.subNames.size(); j++) {
            result += unit.subNames.at(j).toName();
            if (j != (unit.subNames.size() - 1)) {
              result += ",";
            }
          }
          result += "]";
          break;
        }
        case LinkUnitType::name:
        case LinkUnitType::typeName:
        case LinkUnitType::genericPrerunValue:
        case LinkUnitType::genericTypeValue: {
          result += unit.getUsableName();
          break;
        }
      }
    }
    return result;
  }
}

LinkNameUnit::LinkNameUnit(String _name, LinkUnitType _namingType, Maybe<String> _alias, Vec<LinkNames> _subNames)
    : name(_name), unitType(_namingType), alias(_alias), subNames(_subNames) {}

String LinkNameUnit::getUsableName() const { return alias.value_or(name); }

} // namespace qat