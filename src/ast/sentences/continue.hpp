#ifndef QAT_AST_SENTENCES_CONTINUE_HPP
#define QAT_AST_SENTENCES_CONTINUE_HPP

#include "../sentence.hpp"

namespace qat::ast {

class Continue : public Sentence {
  Maybe<Identifier> tag;

public:
  Continue(Maybe<Identifier> _tag, FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::Continue; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif