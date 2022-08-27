#ifndef QAT_AST_EXPRESSIONS_THIS_HPP
#define QAT_AST_EXPRESSIONS_THIS_HPP

#include "../expression.hpp"
#include "../function_prototype.hpp"

namespace qat::ast {

/**
 *  Self represents the pointer to an instance, in the context of a
 * member function
 */
class Self : public Expression {
public:
  /**
   *  Self represents the pointer to an instance, in the context of a
   * member function
   *
   * @param _fileRange
   */
  explicit Self(utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::self; }
};

} // namespace qat::ast

#endif