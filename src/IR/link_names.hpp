#ifndef QAT_IR_LINK_NAMES_HPP
#define QAT_IR_LINK_NAMES_HPP

#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"

namespace qat::IR {
class QatModule;
}

namespace qat {

enum class LinkUnitType {
  lib,
  box,
  type,
  function,
  method,
  variationMethod,
  staticFunction,
  constructor,
  fromConvertor,
  toConvertor,
  defaultConstructor,
  copyConstructor,
  copyAssignment,
  moveConstructor,
  moveAssignment,
  normalBinaryOperator,
  variationBinaryOperator,
  unaryOperator,
  destructor,
  staticField,
  choice,
  mix,
  genericList,
  genericPrerunValue,
  genericTypeValue,
  region,
  skill,
  doType,
  doSkill,
  name,
  typeName,
  global,
};

class LinkNameUnit;

class LinkNames {
public:
  Vec<LinkNameUnit> units;
  Maybe<String>     foreignID;
  IR::QatModule*    parentMod;

  LinkNames();
  LinkNames(Vec<LinkNameUnit> _units, Maybe<String> _foreignID, IR::QatModule* _mod);

  void            addUnit(LinkNameUnit unit, Maybe<String> foreignID);
  useit LinkNames newWith(LinkNameUnit unit, Maybe<String> foreignID);
  useit String    toName() const;
};

class LinkNameUnit {
public:
  String         name;
  LinkUnitType   unitType;
  Maybe<String>  alias;
  Vec<LinkNames> subNames;

  LinkNameUnit(String _name, LinkUnitType _namingType, Maybe<String> _alias = None, Vec<LinkNames> subNames = {});

  useit String getUsableName() const;
};

} // namespace qat

#endif
