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
  explicit FillGeneric(QatType* type) : data(type), kind(FillGenericKind::typed) {}
  explicit FillGeneric(PrerunExpression* expression) : data(expression), kind(FillGenericKind::prerun) {}

  useit static inline FillGeneric* create(QatType* _type) { return std::construct_at(OwnNormal(FillGeneric), _type); }
  useit static inline FillGeneric* create(PrerunExpression* _exp) {
    return std::construct_at(OwnNormal(FillGeneric), _exp);
  }

  useit bool isType() const;
  useit bool isPrerun() const;

  useit QatType*          asType() const;
  useit PrerunExpression* asPrerun() const;

  useit FileRange const& getRange() const;

  useit IR::GenericToFill* toFill(IR::Context* ctx) const;

  useit Json   toJson() const;
  useit String toString() const;
};

} // namespace qat::ast

#endif