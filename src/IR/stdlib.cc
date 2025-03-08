#include "./stdlib.hpp"
#include "./qat_module.hpp"
#include "types/definition.hpp"

namespace qat::ir {

ir::Mod* StdLib::stdLib = nullptr;

ir::DefinitionType* StdLib::stdStringType = nullptr;

bool StdLib::is_std_lib_found() { return stdLib != nullptr; }

bool StdLib::has_string_type() {
	return stdStringType || (is_std_lib_found() && stdLib->has_type_definition("String", AccessInfo::get_privileged()));
}

ir::DefinitionType* StdLib::get_string_type() {
	return stdStringType ? stdStringType : stdLib->get_type_def("String", AccessInfo::get_privileged());
}

} // namespace qat::ir
