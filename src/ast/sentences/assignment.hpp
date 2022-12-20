#ifndef QAT_AST_SENTENCES_ASSIGNMENT_HPP
#define QAT_AST_SENTENCES_ASSIGNMENT_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include "../sentence.hpp"

#include <string>

namespace qat::ast {

class Assignment : public Sentence {
private:
  Expression* lhs;
  Expression* value;

public:
  Assignment(Expression* _lhs, Expression* _value, FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx);
  useit Json       toJson() const;
  useit NodeType   nodeType() const { return NodeType::reassignment; }
};

} // namespace qat::ast

#endif