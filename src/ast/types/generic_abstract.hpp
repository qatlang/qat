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

class TypedGeneric;
class PrerunGeneric;

class GenericAbstractType {
protected:
  usize       index;
  Identifier  name;
  GenericKind kind;
  FileRange   range;

  GenericAbstractType(usize _index, Identifier _name, GenericKind _kind, FileRange _range)
      : index(_index), name(std::move(_name)), kind(_kind), range(std::move(_range)) {
    ast::QatType::generics.push_back(this);
  }

public:
  useit usize      getIndex() const;
  useit Identifier getName() const;
  useit FileRange  getRange() const;

  useit bool           isTyped() const;
  useit TypedGeneric*  asTyped() const;
  useit bool           isPrerun() const;
  useit PrerunGeneric* asPrerun() const;

  virtual void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> expect, IR::EntityState* ent,
                                   IR::Context* ctx) = 0;

  virtual void       emit(IR::Context* ctx) const = 0;
  useit virtual bool hasDefault() const           = 0;
  useit virtual bool isSet() const                = 0;
  virtual void       unset() const                = 0;
  useit virtual Json toJson() const               = 0;

  useit IR::GenericParameter* toIRGenericType() const;

  virtual ~GenericAbstractType() = default;
};

} // namespace qat::ast

#endif