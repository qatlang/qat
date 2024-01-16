#ifndef QAT_AST_SENTENCES_SAY_HPP
#define QAT_AST_SENTENCES_SAY_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

enum class SayType {
  say,
  dbg,
  only,
};

class SayLike : public Sentence {
private:
  Vec<Expression*> expressions;
  SayType          sayType;

public:
  SayLike(SayType _sayTy, Vec<Expression*> _expressions, FileRange _fileRange)
      : Sentence(_fileRange), expressions(_expressions), sayType(_sayTy) {}

  useit static inline SayLike* create(SayType _sayTy, Vec<Expression*> _expressions, FileRange _fileRange) {
    return std::construct_at(OwnNormal(SayLike), _sayTy, _expressions, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::SAY_LIKE; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif