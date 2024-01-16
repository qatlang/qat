#ifndef QAT_AST_SENTENCES_BREAK_HPP
#define QAT_AST_SENTENCES_BREAK_HPP

#include "../sentence.hpp"

namespace qat::ast {

class Break : public Sentence {
  Maybe<Identifier> tag;

public:
  Break(Maybe<Identifier> _tag, FileRange _fileRange) : Sentence(_fileRange), tag(_tag) {}

  useit static inline Break* create(Maybe<Identifier> _tag, FileRange _fileRange) {
    return std::construct_at(OwnNormal(Break), _tag, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::BREAK; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif