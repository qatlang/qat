#ifndef QAT_AST_SENTENCES_CONTINUE_HPP
#define QAT_AST_SENTENCES_CONTINUE_HPP

#include "../sentence.hpp"

namespace qat::ast {

class Continue : public Sentence {
  Maybe<Identifier> tag;

public:
  Continue(Maybe<Identifier> _tag, FileRange _fileRange) : Sentence(_fileRange), tag(_tag) {}

  useit static inline Continue* create(Maybe<Identifier> _tag, FileRange _fileRange) {
    return std::construct_at(OwnNormal(Continue), _tag, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::CONTINUE; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif