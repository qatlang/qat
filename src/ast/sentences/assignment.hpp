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
  Assignment(Expression* _lhs, Expression* _value, FileRange _fileRange)
      : Sentence(_fileRange), lhs(_lhs), value(_value) {}

  useit static inline Assignment* create(Expression* _lhs, Expression* _value, FileRange _fileRange) {
    return std::construct_at(OwnNormal(Assignment), _lhs, _value, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx);
  useit Json       toJson() const;
  useit NodeType   nodeType() const { return NodeType::ASSIGNMENT; }
};

} // namespace qat::ast

#endif