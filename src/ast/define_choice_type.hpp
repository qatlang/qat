#ifndef QAT_AST_DEFINE_CHOICE_TYPE_HPP
#define QAT_AST_DEFINE_CHOICE_TYPE_HPP

#include "./node.hpp"

namespace qat::ast {

class DefineChoiceType : public Node {
public:
  struct Field {
    String           name;
    utils::FileRange range;
  };

  struct Value {
    i64              data;
    utils::FileRange range;
  };

private:
  String                         name;
  Vec<Pair<Field, Maybe<Value>>> fields;
  utils::VisibilityKind          visibility;

public:
  DefineChoiceType(String name, Vec<Pair<Field, Maybe<Value>>> fields,
                   utils::VisibilityKind visibility,
                   utils::FileRange      fileRange);

  void  createType(IR::Context *ctx);
  void  defineType(IR::Context *ctx) final;
  void  define(IR::Context *ctx) final {}
  useit IR::Value *emit(IR::Context *ctx) final { return nullptr; }
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::defineChoiceType; }
};

} // namespace qat::ast

#endif