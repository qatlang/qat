#ifndef QAT_AST_EXPRESSIONS_MOVE_POINTER_HPP
#define QAT_AST_EXPRESSIONS_MOVE_POINTER_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

#include <helpers/vec.hpp>

namespace qat::ast {

enum class CopyMovePtrKind {
	PTR,
	MULTI,
	TO,
	FROM,
	RANGE,
};

class CopyMovePointer : public Sentence {
	Expression*      candidate;
	Vec<Expression*> arguments;
	CopyMovePtrKind  kind;
	bool             isMove;
	bool             shouldDestroyAfter;

  public:
	CopyMovePointer(Expression* _candidate, Vec<Expression*> _arguments, CopyMovePtrKind _kind, bool _isMove,
	                bool _shouldDestroyAfter, FileRangePtr _fileRange)
	    : Sentence(_fileRange), candidate(_candidate), arguments(std::move(_arguments)), kind(_kind), isMove(_isMove),
	      shouldDestroyAfter(_shouldDestroyAfter) {}

	static CopyMovePointer* create(Expression* candidate, Vec<Expression*> arguments, CopyMovePtrKind kind, bool isMove,
	                               bool shouldDestroyAfter, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(CopyMovePointer), candidate, std::move(arguments), kind, isMove,
		                         shouldDestroyAfter, fileRange);
	}

	String kind_to_string(CopyMovePtrKind kindVal) const {
		switch (kindVal) {
			case CopyMovePtrKind::FROM:
				return "from";
			case CopyMovePtrKind::MULTI:
				return "multi";
			case CopyMovePtrKind::PTR:
				return "ptr";
			case CopyMovePtrKind::RANGE:
				return "in";
			case CopyMovePtrKind::TO:
				return "to";
		}
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(candidate);
		for (auto arg : arguments) {
			UPDATE_DEPS(arg);
		}
	}

	ir::Value* emit(EmitCtx* emitCtx) final;

	NodeType nodeType() const final { return NodeType::COPY_MOVE_POINTER; }
};

} // namespace qat::ast

#endif