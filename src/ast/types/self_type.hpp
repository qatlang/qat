#ifndef QAT_AST_TYPES_SELF_TYPE_HPP
#define QAT_AST_TYPES_SELF_TYPE_HPP

#include "qat_type.hpp"

namespace qat::ast {

class SelfType : public QatType {
  friend class MemberPrototype;
  friend class OperatorPrototype;
  bool isJustType;

  bool canBeSelfInstance = false;
  bool isVarRef          = false;

public:
  SelfType(bool isJustType, FileRange _fileRange);

  useit IR::QatType* emit(IR::Context* ctx);
  useit Json         toJson() const;
  useit String       toString() const;
  useit TypeKind     typeKind() const { return TypeKind::selfType; }
};

} // namespace qat::ast

#endif