#ifndef QAT_AST_GENERICS_HPP
#define QAT_AST_GENERICS_HPP

#include "./types/qat_type.hpp"
#include "expression.hpp"
#include "node.hpp"

namespace qat::ast {

enum class FillGenericKind {
  typed,
  prerun,
};

class FillGeneric {
  void*           data;
  FillGenericKind kind;

public:
  explicit FillGeneric(Type* type) : data(type), kind(FillGenericKind::typed) {}
  explicit FillGeneric(PrerunExpression* expression) : data(expression), kind(FillGenericKind::prerun) {}

  useit static FillGeneric* create(Type* _type) { return std::construct_at(OwnNormal(FillGeneric), _type); }
  useit static FillGeneric* create(PrerunExpression* _exp) { return std::construct_at(OwnNormal(FillGeneric), _exp); }

  useit bool is_type() const;
  useit bool is_prerun() const;

  useit Type*             as_type() const;
  useit PrerunExpression* as_prerun() const;

  useit FileRange const& get_range() const;

  useit ir::GenericToFill* toFill(EmitCtx* ctx) const;

  useit Json   to_json() const;
  useit String to_string() const;
};

} // namespace qat::ast

#endif
