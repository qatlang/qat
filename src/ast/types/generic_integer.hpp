#ifndef QAT_AST_TYPES_GENERIC_INTEGER_HPP
#define QAT_AST_TYPES_GENERIC_INTEGER_HPP

#include "qat_type.hpp"

namespace qat::ast {

class GenericIntegerType : public Type {
private:
  PrerunExpression* bitValue;
  Maybe<bool>       isUnsigned;
  PrerunExpression* isUnsignedExp = nullptr;

public:
  GenericIntegerType(PrerunExpression* _bitValue, Maybe<bool> _isUnsigned, PrerunExpression* _isUnsignedExp,
                     FileRange _fileRange)
      : Type(_fileRange), bitValue(_bitValue), isUnsigned(_isUnsigned), isUnsignedExp(_isUnsignedExp) {}

  useit static inline GenericIntegerType* create_specific(PrerunExpression* bitValue, bool isUnsigned,
                                                          FileRange fileRange) {
    return std::construct_at(OwnNormal(GenericIntegerType), bitValue, isUnsigned, nullptr, fileRange);
  }

  useit static inline GenericIntegerType* create(PrerunExpression* bitValue, PrerunExpression* isUnsigned,
                                                 FileRange fileRange) {
    return std::construct_at(OwnNormal(GenericIntegerType), bitValue, None, isUnsigned, fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx) final;

  useit ir::Type*   emit(EmitCtx* ctx) final;
  useit Json        to_json() const final;
  useit String      to_string() const final;
  useit AstTypeKind type_kind() const final { return AstTypeKind::GENERIC_INTEGER; }
};

} // namespace qat::ast

#endif