#ifndef QAT_AST_EXPRESSIONS_THIS_HPP
#define QAT_AST_EXPRESSIONS_THIS_HPP

#include "../expression.hpp"
#include "../function.hpp"

namespace qat::ast {

/**
 *  Self represents the pointer to an instance, in the context of a
 * member function
 */
class Self : public Expression {
public:
  explicit Self(FileRange _fileRange) : Expression(_fileRange) {}

  useit static inline Self* create(FileRange _fileRange) { return std::construct_at(OwnNormal(Self), _fileRange); }

  useit IR::Value* emit(IR::Context* ctx) override;
  useit Json       toJson() const override;
  useit NodeType   nodeType() const override { return NodeType::SELF; }
};

} // namespace qat::ast

#endif