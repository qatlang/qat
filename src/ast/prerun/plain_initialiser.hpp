#ifndef QAT_PRERUN_PLAIN_INITIALISER_HPP
#define QAT_PRERUN_PLAIN_INITIALISER_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunPlainInit final : public PrerunExpression, public TypeInferrable {
  private:
	Maybe<PrerunExpression*> type;
	Maybe<Vec<Identifier>>	 fields;
	Vec<PrerunExpression*>	 fieldValues;

  public:
	PrerunPlainInit(Maybe<PrerunExpression*> _type, Maybe<Vec<Identifier>> _fields, Vec<PrerunExpression*> _fieldValues,
					FileRange _fileRange)
		: PrerunExpression(_fileRange), type(_type), fields(_fields), fieldValues(_fieldValues) {}

	useit static PrerunPlainInit* create(Maybe<PrerunExpression*> _type, Maybe<Vec<Identifier>> _fields,
										 Vec<PrerunExpression*> _fieldValues, FileRange _fileRange) {
		return std::construct_at(OwnNormal(PrerunPlainInit), _type, _fields, _fieldValues, _fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		if (type.has_value()) {
			UPDATE_DEPS(type.value());
		}
		for (auto field : fieldValues) {
			UPDATE_DEPS(field);
		}
	}

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;
	useit NodeType		   nodeType() const final { return NodeType::PRERUN_PLAIN_INITIALISER; }
	useit String		   to_string() const final;
	useit Json			   to_json() const final;
};

} // namespace qat::ast

#endif
