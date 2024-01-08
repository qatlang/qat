#include "./stdlib.hpp"
#include "./qat_module.hpp"
#include "types/definition.hpp"

namespace qat::IR {

IR::QatModule* StdLib::stdLib = nullptr;

IR::DefinitionType* StdLib::stdStringType = nullptr;

bool StdLib::isStdLibFound() { return stdLib != nullptr; }

bool StdLib::hasStringType() {
  return stdStringType || (isStdLibFound() && stdLib->hasTypeDef("String", AccessInfo::GetPrivileged()));
}

IR::DefinitionType* StdLib::getStringType() {
  return stdStringType ? stdStringType : stdLib->getTypeDef("String", AccessInfo::GetPrivileged());
}

} // namespace qat::IR