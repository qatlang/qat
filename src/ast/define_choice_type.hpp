#ifndef QAT_AST_DEFINE_CHOICE_TYPE_HPP
#define QAT_AST_DEFINE_CHOICE_TYPE_HPP

#include "./node.hpp"

namespace qat::ast {

class DefineChoiceType : public Node {
public:
  struct Value {
    i64       data;
    FileRange range;
  };

private:
  Identifier                          name;
  Vec<Pair<Identifier, Maybe<Value>>> fields;
  utils::VisibilityKind               visibility;
  Maybe<usize>                        defaultVal;

public:
  DefineChoiceType(Identifier name, Vec<Pair<Identifier, Maybe<Value>>> fields, Maybe<usize> defaultVal,
                   utils::VisibilityKind visibility, FileRange fileRange);

  void  createType(IR::Context* ctx);
  void  defineType(IR::Context* ctx) final;
  void  define(IR::Context* ctx) final {}
  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::defineChoiceType; }
};

} // namespace qat::ast

#endif