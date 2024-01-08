#ifndef QAT_STD_LIB_QUERY_HPP
#define QAT_STD_LIB_QUERY_HPP

#include "./types/definition.hpp"

namespace qat {
class QatSitter;
}

namespace qat::IR {

class StdLib {
public:
  static IR::QatModule*      stdLib;
  static IR::DefinitionType* stdStringType;

  useit static bool isStdLibFound();

  useit static bool            hasStringType();
  useit static DefinitionType* getStringType();
};

} // namespace qat::IR

#endif