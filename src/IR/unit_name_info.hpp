#ifndef LINK_NAME_INFO_HPP
#define LINK_NAME_INFO_HPP

#include "../utils/helpers.hpp"

namespace qat::IR {
class QatModule;
}

namespace qat::utils {

enum class UnitNameType {
  lib,
  box,
  type,
  TypeDef,
  function,
  method,
  variationMethod,
  staticFunction,
  staticField,
  choice,
  mix,
  genericList,
  genericConstValue,
  genericTypeValue,
  region,
};

class UnitNameInfo {
public:
  String               name;
  UnitNameType         namingType;
  Maybe<String>        alias;
  const IR::QatModule* relevantParent;
  bool                 overrideParentNames;
  Vec<UnitNameInfo>    subUnits;

  UnitNameInfo(String _name, UnitNameType _namingType, const IR::QatModule* _relevantParent, Maybe<String> _alias,
               bool _overrideParentName = false)
      : name(_name), namingType(_namingType), alias(_alias), relevantParent(_relevantParent),
        overrideParentNames(_overrideParentName) {}

  String getUsableName() const { return alias.value_or(name); }
};

} // namespace qat::utils

#endif
