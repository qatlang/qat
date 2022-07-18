#ifndef QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP
#define QAT_AST_EXPRESSIONS_TUPLE_VALUE_HPP

#include "../expression.hpp"

#include <vector>

namespace qat {
namespace AST {
class TupleValue : public Expression {
  std::vector<Expression *> members;

public:
  TupleValue(std::vector<Expression *> _members,
             utils::FilePlacement _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::tupleValue; }
};
} // namespace AST
} // namespace qat

#endif