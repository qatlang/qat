#ifndef QAT_AST_EXPRESSIONS_COPY_HPP
#define QAT_AST_EXPRESSIONS_COPY_HPP

#include "../expression.hpp"

namespace qat::ast {

class Copy final : public Expression, public LocalDeclCompatible, public InPlaceCreatable {
  friend class Assignment;
  friend class LocalDeclaration;

private:
  Expression* exp;
  bool        isExpSelf = false;

  bool isAssignment = false;

public:
  Copy(Expression* _exp, bool _isExpSelf, FileRange _fileRange)
      : Expression(std::move(_fileRange)), exp(_exp), isExpSelf(_isExpSelf) {}

  useit static Copy* create(Expression* exp, bool _isExpSelf, FileRange fileRange) {
    return std::construct_at(OwnNormal(Copy), exp, _isExpSelf, fileRange);
  }

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  IN_PLACE_CREATABLE_FUNCTIONS

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(exp);
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::COPY; }
  useit Json       to_json() const final;
};

} // namespace qat::ast

#endif
