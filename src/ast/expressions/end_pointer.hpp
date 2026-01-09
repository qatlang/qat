#ifndef QAT_AST_EXPRESSIONS_END_POINTER_HPP
#define QAT_AST_EXPRESSIONS_END_POINTER_HPP

#include "../expression.hpp"

namespace qat::ast {

enum class EndPointerKind {
	PTR,
	MULTI,
	TO,
	FROM,
	RANGE,
};

class EndPointer : public Expression {
  private:
	Expression*      candidate;
	Vec<Expression*> args;
	EndPointerKind   kind;

  public:
	EndPointer(Expression* _candidate, EndPointerKind _kind, Vec<Expression*> _args, FileRangePtr _fileRange)
	    : Expression(_fileRange), candidate(_candidate), args(std::move(_args)), kind(_kind) {}

	useit static EndPointer* create(Expression* candidate, EndPointerKind kind, Vec<Expression*> args,
	                                FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(EndPointer), candidate, kind, std::move(args), fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(candidate);
	}

	useit String kind_to_string() const {
		switch (kind) {
			case EndPointerKind::PTR:
				return "pointer";
			case EndPointerKind::MULTI:
				return "multi";
			case EndPointerKind::FROM:
				return "from";
			case EndPointerKind::TO:
				return "upto";
			case EndPointerKind::RANGE:
				return "range";
		}
	}

	useit ir::Value* emit(EmitCtx* emitCtx) final;

	useit NodeType nodeType() const final { return NodeType::END_POINTER; }
};

} // namespace qat::ast

#endif
