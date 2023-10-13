#ifndef QAT_IR_GENERICS_HPP
#define QAT_IR_GENERICS_HPP

#include "../utils/identifier.hpp"
#include "./types/qat_type.hpp"

namespace qat::ast {
class GenericAbstractType;
}

namespace qat::IR {

enum class GenericKind {
  typedGeneric,
  constGeneric,
};

class TypedGeneric;
class PrerunGeneric;
class PrerunValue;
class GenericToFill;

void fillGenerics(IR::Context* ctx, Vec<ast::GenericAbstractType*>& genAbs, Vec<GenericToFill*>& fills,
                  FileRange const& fileRange);

class GenericToFill {
  void*       data = nullptr;
  GenericKind kind;
  FileRange   range;

  GenericToFill(void* _data, GenericKind _kind, FileRange _range);

public:
  useit static GenericToFill* get(IR::PrerunValue* constVal, FileRange range);
  useit static GenericToFill* get(IR::QatType* type, FileRange range);

  useit bool isPrerun() const;
  useit IR::PrerunValue* asPrerun() const;

  useit bool isType() const;
  useit IR::QatType* asType() const;

  useit FileRange getRange() const;

  useit String toString() const;
};

class GenericParameter {
protected:
  Identifier  name;
  GenericKind kind;
  FileRange   range;

  GenericParameter(Identifier name, GenericKind kind, FileRange range);

public:
  useit Identifier getName() const;
  useit FileRange  getRange() const;
  useit bool       isSame(const String& name) const;

  useit bool          isTyped() const;
  useit TypedGeneric* asTyped() const;

  useit bool           isPrerun() const;
  useit PrerunGeneric* asPrerun() const;

  useit bool isEqualTo(GenericToFill* fill) const;
};

class TypedGeneric : public GenericParameter {
  IR::QatType* type;

  TypedGeneric(Identifier name, IR::QatType* type, FileRange range);

public:
  useit static TypedGeneric* get(Identifier name, IR::QatType* type, FileRange range);

  useit IR::QatType* getType() const;
};

class PrerunGeneric : public GenericParameter {
  IR::PrerunValue* constant;

  PrerunGeneric(Identifier name, IR::PrerunValue* constant, FileRange range);

public:
  useit static PrerunGeneric* get(Identifier name, IR::PrerunValue* type, FileRange range);

  useit IR::PrerunValue* getExpression() const;
  useit IR::QatType* getType() const;
};

} // namespace qat::IR

#endif