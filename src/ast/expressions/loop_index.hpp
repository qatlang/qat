#ifndef QAT_AST_EXPRESSIONS_LOOP_INDEX_HPP
#define QAT_AST_EXPRESSIONS_LOOP_INDEX_HPP

#include "../expression.hpp"

namespace qat::ast {

class TagOfLoop final : public Expression {
  String indexName;

public:
  TagOfLoop(String _indexName, FileRange _fileRange) : Expression(_fileRange), indexName(_indexName) {}

  useit static inline TagOfLoop* create(String _indexName, FileRange _fileRange) {
    return std::construct_at(OwnNormal(TagOfLoop), _indexName, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {}

  useit bool hasName() const;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::TAG_OF_LOOP; }
};

} // namespace qat::ast

#endif