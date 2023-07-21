#ifndef QAT_AST_SENTENCES_LOCAL_DECLARATION_HPP
#define QAT_AST_SENTENCES_LOCAL_DECLARATION_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include "../types/qat_type.hpp"
#include <optional>

namespace qat::ast {

class LocalDeclaration : public Sentence {
private:
  QatType*           type = nullptr;
  Identifier         name;
  Maybe<Expression*> value;
  bool               variability;
  bool               isRef;

public:
  LocalDeclaration(QatType* _type, bool isRef, Identifier _name, Maybe<Expression*> _value,
                   bool _variability, FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) override;
  useit Json       toJson() const override;
  useit NodeType   nodeType() const override { return NodeType::localDeclaration; }
};

} // namespace qat::ast

#endif