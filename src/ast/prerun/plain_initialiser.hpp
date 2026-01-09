#ifndef QAT_PRERUN_PLAIN_INITIALISER_HPP
#define QAT_PRERUN_PLAIN_INITIALISER_HPP

#include "../expression.hpp"
#include "../type_like.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunPlainInit final : public PrerunExpression, public TypeInferrable {
	TypeLike               type;
	Maybe<Vec<Identifier>> fields;
	Vec<PrerunExpression*> fieldValues;

  public:
	PrerunPlainInit(TypeLike _type, Maybe<Vec<Identifier>> _fields, Vec<PrerunExpression*> _fieldValues,
	                FileRangePtr _fileRange)
	    : PrerunExpression(_fileRange), type(_type), fields(_fields), fieldValues(_fieldValues) {}

	static PrerunPlainInit* create(TypeLike _type, Maybe<Vec<Identifier>> _fields, Vec<PrerunExpression*> _fieldValues,
	                               FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(PrerunPlainInit), _type, _fields, _fieldValues, _fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		type.update_dependencies(phase, dep, ent, ctx);
		for (auto field : fieldValues) {
			UPDATE_DEPS(field);
		}
	}

	ir::PrerunValue* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::PRERUN_PLAIN_INITIALISER; }

	String to_string() const final;
};

} // namespace qat::ast

#endif
