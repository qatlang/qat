#ifndef QAT_AST_EXPRESSIONS_THIS_HPP
#define QAT_AST_EXPRESSIONS_THIS_HPP

#include "../expression.hpp"
#include "../function_prototype.hpp"

namespace qat {
namespace AST {

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
   * @param _filePlacement
   */
  Self(utils::FilePlacement _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return qat::AST::NodeType::selfExpression; }
};

} // namespace AST
} // namespace qat

#endif