#ifndef QAT_AST_SENTENCES_CONTINUE_HPP
#define QAT_AST_SENTENCES_CONTINUE_HPP

#include "../sentence.hpp"

namespace qat::ast {

class Continue : public Sentence {
  Maybe<String> tag;

public:
  Continue(Maybe<String> _tag, utils::FileRange _fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType   nodeType() const final { return NodeType::Continue; }
  useit nuo::Json toJson() const final;
};

} // namespace qat::ast

#endif