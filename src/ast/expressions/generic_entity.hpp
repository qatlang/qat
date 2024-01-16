#ifndef QAT_AST_EXPRESSIONS_GENERIC_ENTITY_HPP
#define QAT_AST_EXPRESSIONS_GENERIC_ENTITY_HPP

#include "../expression.hpp"
#include "../generics.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class GenericEntity : public Expression {
private:
  u32               relative;
  Vec<Identifier>   names;
  Vec<FillGeneric*> genericTypes;

public:
  GenericEntity(u32 _relative, Vec<Identifier> _names, Vec<FillGeneric*> _genericTypes, FileRange _fileRange)
      : Expression(_fileRange), relative(_relative), names(_names), genericTypes(_genericTypes) {}

  useit static inline GenericEntity* create(u32 _relative, Vec<Identifier> _names, Vec<FillGeneric*> _genericTypes,
                                            FileRange _fileRange) {
    return std::construct_at(OwnNormal(GenericEntity), _relative, _names, _genericTypes, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::GENERIC_ENTITY; }
};

} // namespace qat::ast

#endif