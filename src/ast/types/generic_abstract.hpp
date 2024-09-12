#ifndef QAT_AST_TYPES_GENERIC_ABSTRACT_HPP
#define QAT_AST_TYPES_GENERIC_ABSTRACT_HPP

#include "../../IR/context.hpp"
#include "../../utils/identifier.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

enum class GenericKind {
  typedGeneric,
  prerunGeneric,
};

class TypedGenericAbstract;
class PrerunGenericAbstract;

class GenericAbstractType {
protected:
  usize       index;
  Identifier  name;
  GenericKind kind;
  FileRange   range;

  GenericAbstractType(usize _index, Identifier _name, GenericKind _kind, FileRange _range)
      : index(_index), name(std::move(_name)), kind(_kind), range(std::move(_range)) {
    ast::Type::generics.push_back(this);
  }

public:
  useit usize      getIndex() const;
  useit Identifier get_name() const;
  useit FileRange  get_range() const;

  useit bool                   is_typed() const;
  useit TypedGenericAbstract*  as_typed() const;
  useit bool                   is_prerun() const;
  useit PrerunGenericAbstract* as_prerun() const;

  virtual void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                   EmitCtx* ctx) = 0;

  virtual void       emit(EmitCtx* ctx) const = 0;
  useit virtual bool hasDefault() const       = 0;
  useit virtual bool isSet() const            = 0;
  virtual void       unset() const            = 0;
  useit virtual Json to_json() const          = 0;

  useit ir::GenericArgument* toIRGenericType() const;

  virtual ~GenericAbstractType() = default;
};

} // namespace qat::ast

#endif
