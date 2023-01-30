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
class ConstGeneric;
class ConstantValue;
class GenericToFill;

void fillGenerics(IR::Context* ctx, Vec<ast::GenericAbstractType*>& genAbs, Vec<GenericToFill*>& fills,
                  FileRange const& fileRange);

class GenericToFill {
  void*       data = nullptr;
  GenericKind kind;
  FileRange   range;

  GenericToFill(void* _data, GenericKind _kind, FileRange _range);

public:
  useit static GenericToFill* get(IR::ConstantValue* constVal, FileRange range);
  useit static GenericToFill* get(IR::QatType* type, FileRange range);

  useit bool isConst() const;
  useit IR::ConstantValue* asConst() const;

  useit bool isType() const;
  useit IR::QatType* asType() const;

  useit FileRange getRange() const;

  useit String toString() const;
};

class GenericType {
protected:
  Identifier  name;
  GenericKind kind;
  FileRange   range;

  GenericType(Identifier name, GenericKind kind, FileRange range);

public:
  useit Identifier getName() const;
  useit FileRange  getRange() const;
  useit bool       isSame(const String& name) const;

  useit bool          isTyped() const;
  useit TypedGeneric* asTyped() const;

  useit bool          isConst() const;
  useit ConstGeneric* asConst() const;

  useit bool isEqualTo(GenericToFill* fill) const;
};

class TypedGeneric : public GenericType {
  IR::QatType* type;

  TypedGeneric(Identifier name, IR::QatType* type, FileRange range);

public:
  useit static TypedGeneric* get(Identifier name, IR::QatType* type, FileRange range);

  useit IR::QatType* getType() const;
};

class ConstGeneric : public GenericType {
  IR::ConstantValue* constant;

  ConstGeneric(Identifier name, IR::ConstantValue* constant, FileRange range);

public:
  useit static ConstGeneric* get(Identifier name, IR::ConstantValue* type, FileRange range);

  useit IR::ConstantValue* getExpression() const;
  useit IR::QatType* getType() const;
};

} // namespace qat::IR

#endif