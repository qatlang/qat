#ifndef QAT_AST_SENTENCES_LOCAL_DECLARATION_HPP
#define QAT_AST_SENTENCES_LOCAL_DECLARATION_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include "../types/qat_type.hpp"
#include <optional>

namespace qat::ast {

class LocalDeclaration final : public Sentence {
private:
  Type*              type = nullptr;
  Identifier         name;
  Maybe<Expression*> value;
  bool               variability;
  bool               isRef;

public:
  LocalDeclaration(Type* _type, bool _isRef, Identifier _name, Maybe<Expression*> _value, bool _variability,
                   FileRange _fileRange)
      : Sentence(_fileRange), type(_type), name(_name), value(_value), variability(_variability), isRef(_isRef) {}

  useit static LocalDeclaration* create(Type* _type, bool _isRef, Identifier _name, Maybe<Expression*> _value,
                                        bool _variability, FileRange _fileRange) {
    return std::construct_at(OwnNormal(LocalDeclaration), _type, _isRef, _name, _value, _variability, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    if (type) {
      UPDATE_DEPS(type);
    }
    if (value.has_value()) {
      UPDATE_DEPS(value.value());
    }
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit Json       to_json() const final;
  useit NodeType   nodeType() const final { return NodeType::LOCAL_DECLARATION; }
};

} // namespace qat::ast

#endif
