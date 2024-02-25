#ifndef QAT_AST_TYPES_SELF_TYPE_HPP
#define QAT_AST_TYPES_SELF_TYPE_HPP

#include "qat_type.hpp"

namespace qat::ast {

class SelfType final : public QatType {
  friend class MemberPrototype;
  friend class OperatorPrototype;
  bool isJustType;

  bool canBeSelfInstance = false;
  bool isVarRef          = false;

public:
  SelfType(bool _isJustType, FileRange _fileRange) : QatType(_fileRange), isJustType(_isJustType) {}

  useit static inline SelfType* create(bool _isJustType, FileRange _fileRange) {
    return std::construct_at(OwnNormal(SelfType), _isJustType, _fileRange);
  }

  useit IR::QatType* emit(IR::Context* ctx);
  useit Json         toJson() const;
  useit String       toString() const;
  useit AstTypeKind  typeKind() const { return AstTypeKind::SELF_TYPE; }
};

} // namespace qat::ast

#endif