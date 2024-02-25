#ifndef QAT_AST_MEMBER_INITIATLISATION_HPP
#define QAT_AST_MEMBER_INITIATLISATION_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class MemberInit final : public Sentence {
  friend class ConstructorDefinition;
  friend class ConvertorDefinition;

  Identifier  memName;
  Expression* value;
  bool        isInitOfMixVariantWithoutValue;

  bool isAllowed = false;

public:
  MemberInit(Identifier _memName, Expression* _value, bool _isInitOfMixVariantWithoutValue, FileRange _fileRange)
      : Sentence(_fileRange), memName(_memName), value(_value),
        isInitOfMixVariantWithoutValue(_isInitOfMixVariantWithoutValue) {}

  useit static inline MemberInit* create(Identifier _memName, Expression* _value, bool _isInitOfMixVariantWithoutValue,
                                         FileRange _fileRange) {
    return std::construct_at(OwnNormal(MemberInit), _memName, _value, _isInitOfMixVariantWithoutValue, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(value);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::MEMBER_INIT; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif