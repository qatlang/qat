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
  GenericEntity(u32 _relative, Vec<Identifier> _names, Vec<FillGeneric*> _genericTypes, FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::genericEntity; }
};

} // namespace qat::ast

#endif