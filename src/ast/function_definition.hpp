#ifndef QAT_AST_FUNCTION_DEFINITION_HPP
#define QAT_AST_FUNCTION_DEFINITION_HPP

#include "../IR/context.hpp"
#include "./function_prototype.hpp"
#include "./node.hpp"
#include "./sentence.hpp"
#include <iostream>
#include <string>

namespace qat::ast {

class FunctionDefinition : public Node {
private:
  Vec<Sentence *> sentences;

public:
  FunctionDefinition(FunctionPrototype *_prototype, Vec<Sentence *> _sentences,
                     utils::FileRange _fileRange);

  FunctionPrototype *prototype;

  IR::Value *emit(IR::Context *ctx);

  NodeType nodeType() const { return NodeType::functionDefinition; }

  nuo::Json toJson() const;
};

} // namespace qat::ast

#endif
