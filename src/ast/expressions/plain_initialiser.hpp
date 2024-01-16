#ifndef QAT_AST_PLAIN_INITIALISER_HPP
#define QAT_AST_PLAIN_INITIALISER_HPP

#include "../expression.hpp"
#include "../types/named.hpp"

namespace qat::ast {

class PlainInitialiser : public Expression, public LocalDeclCompatible {
  friend class LocalDeclaration;

private:
  QatType*                     type;
  Vec<Pair<String, FileRange>> fields;
  Vec<u64>                     indices;
  Vec<Expression*>             fieldValues;

public:
  PlainInitialiser(QatType* _type, Vec<Pair<String, FileRange>> _fields, Vec<Expression*> _fieldValues,
                   FileRange _fileRange)
      : Expression(_fileRange), type(_type), fields(_fields), fieldValues(_fieldValues) {}

  useit static inline PlainInitialiser* create(QatType* _type, Vec<Pair<String, FileRange>> _fields,
                                               Vec<Expression*> _fieldValues, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PlainInitialiser), _type, _fields, _fieldValues, _fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::PLAIN_INITIALISER; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif