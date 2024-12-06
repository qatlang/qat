#ifndef QAT_AST_SENTENCES_ASSIGNMENT_HPP
#define QAT_AST_SENTENCES_ASSIGNMENT_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class Assignment final : public Sentence {
private:
  Expression* lhs;
  Expression* value;

public:
  Assignment(Expression* _lhs, Expression* _value, FileRange _fileRange)
      : Sentence(_fileRange), lhs(_lhs), value(_value) {}

  useit static Assignment* create(Expression* _lhs, Expression* _value, FileRange _fileRange) {
    return std::construct_at(OwnNormal(Assignment), _lhs, _value, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(lhs);
    UPDATE_DEPS(value);
  }

  useit ir::Value* emit(EmitCtx* ctx);
  useit Json       to_json() const;
  useit NodeType   nodeType() const { return NodeType::ASSIGNMENT; }
};

} // namespace qat::ast

#endif
