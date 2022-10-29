#ifndef QAT_AST_SENTENCES_SAY_HPP
#define QAT_AST_SENTENCES_SAY_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

enum class SayType {
  say,
  dbg,
};

class SayLike : public Sentence {
private:
  Vec<Expression*> expressions;
  SayType          sayType;

public:
  SayLike(SayType _sayTy, Vec<Expression*> _expressions, utils::FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::saySentence; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif