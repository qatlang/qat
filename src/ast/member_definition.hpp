#ifndef QAT_AST_MEMBER_DEFINITION_HPP
#define QAT_AST_MEMBER_DEFINITION_HPP

#include "../IR/context.hpp"
#include "./member_prototype.hpp"
#include "./node.hpp"
#include "./sentence.hpp"
#include <iostream>
#include <string>

namespace qat::ast {

class MemberDefinition : public Node {
private:
  Vec<Sentence *>  sentences;
  MemberPrototype *prototype;

public:
  MemberDefinition(MemberPrototype *_prototype, Vec<Sentence *> _sentences,
                   utils::FileRange _fileRange);

  void  setCoreType(IR::CoreType *coreType);
  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::memberDefinition; }
};

} // namespace qat::ast

#endif
