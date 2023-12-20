#ifndef QAT_AST_MEMBER_INITIATLISATION_HPP
#define QAT_AST_MEMBER_INITIATLISATION_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class MemberInit : public Sentence {
  friend class ConstructorDefinition;
  friend class ConvertorDefinition;

  Identifier  memName;
  Expression* value;
  bool        isInitOfMixVariantWithoutValue;

  bool isAllowed = false;

public:
  MemberInit(Identifier _memName, Expression* _value, bool _isInitOfMixVariantWithoutValue, FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::memberInit; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif