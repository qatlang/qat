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
  LocalDeclaration(QatType* _type, bool _isRef, Identifier _name, Maybe<Expression*> _value, bool _variability,
                   FileRange _fileRange)
      : Sentence(_fileRange), type(_type), name(_name), value(_value), variability(_variability), isRef(_isRef) {}

  useit static inline LocalDeclaration* create(QatType* _type, bool _isRef, Identifier _name, Maybe<Expression*> _value,
                                               bool _variability, FileRange _fileRange) {
    return std::construct_at(OwnNormal(LocalDeclaration), _type, _isRef, _name, _value, _variability, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::LOCAL_DECLARATION; }
};

} // namespace qat::ast

#endif