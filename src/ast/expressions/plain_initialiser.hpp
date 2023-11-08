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
                   FileRange _fileRange);

  LOCAL_DECL_COMPATIBLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::plainInitialiser; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif