#ifndef QAT_AST_CONVERTOR_DEFINITION_HPP
#define QAT_AST_CONVERTOR_DEFINITION_HPP

#include "../IR/context.hpp"
#include "./convertor_prototype.hpp"
#include "./node.hpp"
#include "./sentence.hpp"
#include <iostream>
#include <string>

namespace qat::ast {

class ConvertorDefinition : public Node {
private:
  Vec<Sentence *>     sentences;
  ConvertorPrototype *prototype;

public:
  ConvertorDefinition(ConvertorPrototype *_prototype,
                      Vec<Sentence *> _sentences, utils::FileRange _fileRange);

  void  setCoreType(IR::CoreType *coreType);
  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType nodeType() const final { return NodeType::functionDefinition; }
};

} // namespace qat::ast

#endif
