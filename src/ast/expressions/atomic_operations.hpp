#ifndef QAT_AST_EXPRESSIONS_ATOMIC_OPERATIONS_HPP
#define QAT_AST_EXPRESSIONS_ATOMIC_OPERATIONS_HPP

#include "../expression.hpp"

namespace qat::ast {

enum class AtomicOps {
	EXCHANGE,
	COMPARE_AND_EXCHANGE,
	ADD,
	SUB,
	OR,
	AND,
	NAND,
	XOR,
	MAX,
	MIN,
	INCREMENT_WRAP,
	DECREMENT_WRAP,
	SUBTRACT_CONDITION,
	SUBTRACT_SATURATED,
};

class AtomicOperations : public Expression {
	AtomicOps              ops;
	Expression*            candidate;
	Vec<PrerunExpression*> ordering;
	Vec<Expression*>       arguments;

  public:
	AtomicOperations(AtomicOps _ops, Expression* _candidate, Vec<PrerunExpression*> _ordering,
	                 Vec<Expression*> _arguments, FileRangePtr _fileRange)
	    : Expression(_fileRange), ops(_ops), candidate(_candidate), ordering(_ordering),
	      arguments(std::move(_arguments)) {}

	useit static AtomicOperations* create(AtomicOps ops, Expression* candidate, Vec<PrerunExpression*> ordering,
	                                      Vec<Expression*> arguments, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(AtomicOperations), ops, candidate, ordering, std::move(arguments),
		                         fileRange);
	}

	useit static llvm::AtomicOrdering parse_atomic_ordering(String const& str, FileRangePtr orderRange, EmitCtx* ctx) {
		if (str == "unordered") {
			return llvm::AtomicOrdering::Unordered;
		} else if (str == "relaxed") {
			return llvm::AtomicOrdering::Monotonic;
		} else if (str == "acquire") {
			return llvm::AtomicOrdering::Acquire;
		} else if (str == "release") {
			return llvm::AtomicOrdering::Release;
		} else if (str == "acquire_release") {
			return llvm::AtomicOrdering::AcquireRelease;
		} else if (str == "sequentially_consistent") {
			return llvm::AtomicOrdering::SequentiallyConsistent;
		} else {
			ctx->Error("Unexpected atomic ordering " + ctx->color(str) + " found here", orderRange);
			std::unreachable();
		}
	}

	useit static String operation_to_string(AtomicOps operation) {
		switch (operation) {
			case AtomicOps::EXCHANGE:
				return "exchange";
			case AtomicOps::COMPARE_AND_EXCHANGE:
				return "compare_exchange";
		}
	}

	useit static Maybe<AtomicOps> operation_from_string(String value) {
		if (value == "exchange") {
			return AtomicOps::EXCHANGE;
		} else if (value == "compare_exchange") {
			return AtomicOps::COMPARE_AND_EXCHANGE;
		}
		return None;
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(candidate);
		for (auto* ord : ordering) {
			UPDATE_DEPS(ord);
		}
		for (auto* arg : arguments) {
			UPDATE_DEPS(arg);
		}
	}

	useit ir::Value* emit(EmitCtx* emitCtx) final;

	useit NodeType nodeType() const final { return NodeType::ATOMIC_OPERATIONS; }

	useit Json to_json() const final {
		Vec<JsonValue> argsJSON;
		for (auto* arg : arguments) {
			argsJSON.push_back(arg->to_json());
		}
		Vec<JsonValue> ordersJSON;
		for (auto* ord : ordering) {
			ordersJSON.push_back(ord->to_json());
		}
		return Json()
		    ._("operation", operation_to_string(ops))
		    ._("candidate", candidate->to_json())
		    ._("orderings", ordersJSON)
		    ._("arguments", argsJSON)
		    ._("fileRange", fileRange->to_json());
	}
};

} // namespace qat::ast

#endif
