#ifndef QAT_AST_SENTENCES_LOCAL_DECLARATION_HPP
#define QAT_AST_SENTENCES_LOCAL_DECLARATION_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include "../types/qat_type.hpp"
#include <optional>

namespace qat::ast {

class LocalDeclaration : public Sentence {
private:
  QatType*    type;
  String      name;
  Expression* value;
  bool        variability;
  bool        isRef;
  bool        isPtr;

public:
  LocalDeclaration(QatType* _type, bool isRef, bool isPtr, String _name, Expression* _value, bool _variability,
                   utils::FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) override;
  useit Json       toJson() const override;
  useit NodeType   nodeType() const override { return NodeType::localDeclaration; }
};

} // namespace qat::ast

#endif