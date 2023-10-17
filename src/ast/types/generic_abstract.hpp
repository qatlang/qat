#ifndef QAT_AST_TYPES_GENERIC_ABSTRACT_HPP
#define QAT_AST_TYPES_GENERIC_ABSTRACT_HPP

#include "../../IR/context.hpp"
#include "../../utils/identifier.hpp"

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

  GenericAbstractType(usize index, Identifier name, GenericKind kind, FileRange range);

public:
  useit usize      getIndex() const;
  useit Identifier getName() const;
  useit FileRange  getRange() const;

  useit bool           isTyped() const;
  useit TypedGeneric*  asTyped() const;
  useit bool           isPrerun() const;
  useit PrerunGeneric* asPrerun() const;

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