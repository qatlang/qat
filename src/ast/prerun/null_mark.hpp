#ifndef QAT_AST_PRERUN_NULL_MARK_HPP
#define QAT_AST_PRERUN_NULL_MARK_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class NullMark final : public PrerunExpression, public TypeInferrable {
	Maybe<ast::Type*> providedType;

  public:
	NullMark(Maybe<ast::Type*> _providedType, FileRange _fileRange)
		: PrerunExpression(_fileRange), providedType(_providedType) {}

	useit static NullMark* create(Maybe<ast::Type*> _providedType, FileRange _fileRange) {
		return std::construct_at(OwnNormal(NullMark), _providedType, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		if (providedType.has_value()) {
			UPDATE_DEPS(providedType.value());
		}
	}

	TYPE_INFERRABLE_FUNCTIONS

	useit ir::PrerunValue* emit(EmitCtx* ctx) override;
	useit Json			   to_json() const override;
	useit String		   to_string() const final;
	useit NodeType		   nodeType() const override { return NodeType::NULL_MARK; }
};

} // namespace qat::ast

#endif
