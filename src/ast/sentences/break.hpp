#ifndef QAT_AST_SENTENCES_BREAK_HPP
#define QAT_AST_SENTENCES_BREAK_HPP

#include "../sentence.hpp"

namespace qat::ast {

class Break : public Sentence {
  Maybe<Identifier> tag;

public:
  Break(Maybe<Identifier> _tag, FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::Break; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif