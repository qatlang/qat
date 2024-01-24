#ifndef QAT_AST_ADDRESS_OF_HPP
#define QAT_AST_ADDRESS_OF_HPP

#include "../expression.hpp"

namespace qat::ast {

class AddressOf : public Expression {
  Expression* instance;

public:
  AddressOf(Expression* _instance, FileRange _fileRange) : Expression(_fileRange), instance(_instance) {}

  useit static inline AddressOf* create(Expression* _instance, FileRange _fileRange) {
    return std::construct_at(OwnNormal(AddressOf), _instance, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::ADDRESS_OF; }
};

} // namespace qat::ast

#endif