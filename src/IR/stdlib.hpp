#ifndef QAT_STD_LIB_QUERY_HPP
#define QAT_STD_LIB_QUERY_HPP

#include "./types/definition.hpp"

namespace qat {
class QatSitter;
}

namespace qat::ir {

class StdLib {
	static ir::DefinitionType* stdStringType;

  public:
	static ir::Mod* stdLib;

	useit static bool is_std_lib_found();

	useit static bool            has_string_type();
	useit static DefinitionType* get_string_type();
};

} // namespace qat::ir

#endif