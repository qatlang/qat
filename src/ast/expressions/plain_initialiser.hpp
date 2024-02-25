#ifndef QAT_AST_PLAIN_INITIALISER_HPP
#define QAT_AST_PLAIN_INITIALISER_HPP

#include "../expression.hpp"
#include "../types/named.hpp"

namespace qat::ast {

class PlainInitialiser final : public Expression, public LocalDeclCompatible, public TypeInferrable {
  friend class LocalDeclaration;

private:
  Maybe<QatType*>              type;
  Vec<Pair<String, FileRange>> fields;
  Vec<u64>                     indices;
  Vec<Expression*>             fieldValues;

public:
  PlainInitialiser(Maybe<QatType*> _type, Vec<Pair<String, FileRange>> _fields, Vec<Expression*> _fieldValues,
                   FileRange _fileRange)
      : Expression(_fileRange), type(_type), fields(_fields), fieldValues(_fieldValues) {}

  useit static inline PlainInitialiser* create(Maybe<QatType*> _type, Vec<Pair<String, FileRange>> _fields,
                                               Vec<Expression*> _fieldValues, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PlainInitialiser), _type, _fields, _fieldValues, _fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    if (type) {
      UPDATE_DEPS(type.value());
    }
    for (auto field : fieldValues) {
      UPDATE_DEPS(field);
    }
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::PLAIN_INITIALISER; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif