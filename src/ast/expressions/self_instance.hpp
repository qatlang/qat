#ifndef QAT_AST_EXPRESSIONS_THIS_HPP
#define QAT_AST_EXPRESSIONS_THIS_HPP

#include "../expression.hpp"
#include "../function.hpp"

namespace qat::ast {

class SelfInstance : public Expression {
public:
  explicit SelfInstance(FileRange _fileRange) : Expression(_fileRange) {}

  useit static inline SelfInstance* create(FileRange _fileRange) {
    return std::construct_at(OwnNormal(SelfInstance), _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) override;
  useit Json       toJson() const override;
  useit NodeType   nodeType() const override { return NodeType::SELF; }
};

} // namespace qat::ast

#endif