#ifndef QAT_AST_FUNCTION_DEFINITION_HPP
#define QAT_AST_FUNCTION_DEFINITION_HPP

#include "../IR/context.hpp"
#include "./function_prototype.hpp"
#include "./node.hpp"
#include "./sentence.hpp"
#include <iostream>
#include <string>

namespace qat {
namespace AST {
class FunctionDefinition : public Node {
private:
  std::vector<Sentence *> sentences;

public:
  FunctionDefinition(FunctionPrototype *_prototype,
                     std::vector<Sentence *> _sentences,
                     utils::FilePlacement _filePlacement);

  FunctionPrototype *prototype;

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  NodeType nodeType() const { return NodeType::functionDefinition; }

  nuo::Json toJson() const;
};
} // namespace AST
} // namespace qat

#endif
