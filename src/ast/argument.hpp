#ifndef QAT_AST_ARGUMENT_HPP
#define QAT_AST_ARGUMENT_HPP

#include "../utils/file_range.hpp"
#include "../utils/macros.hpp"
#include "./types/qat_type.hpp"
#include <optional>
#include <string>

namespace qat::ast {

class Argument {
private:
  bool       isVar;
  Identifier name;
  QatType*   type;
  bool       isMember;

  Argument(Identifier _name, bool _isVar, QatType* _type, bool _isMember);

public:
  static Argument* Normal(Identifier name, bool isVar, QatType* type);
  static Argument* ForConstructor(Identifier name, bool isVar, QatType* type, bool isMember);

  useit Identifier getName() const;
  useit bool       isVariable() const;
  useit QatType*   getType();
  useit bool       isTypeMember() const;
};

} // namespace qat::ast

#endif