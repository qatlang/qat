#ifndef QAT_AST_PLAIN_INITIALISER_HPP
#define QAT_AST_PLAIN_INITIALISER_HPP

#include "../expression.hpp"
#include "../type_like.hpp"

namespace qat::ast {

class PlainInitialiser final : public Expression,
                               public LocalDeclCompatible,
                               public InPlaceCreatable,
                               public TypeInferrable {
	friend class LocalDeclaration;

  private:
	TypeLike                     type;
	Vec<Pair<String, FileRange>> fields;
	Vec<u64>                     indices;
	Vec<Expression*>             fieldValues;

  public:
	PlainInitialiser(TypeLike _type, Vec<Pair<String, FileRange>> _fields, Vec<Expression*> _fieldValues,
	                 FileRange _fileRange)
	    : Expression(_fileRange), type(_type), fields(_fields), fieldValues(_fieldValues) {}

	useit static PlainInitialiser* create(TypeLike _type, Vec<Pair<String, FileRange>> _fields,
	                                      Vec<Expression*> _fieldValues, FileRange _fileRange) {
		return std::construct_at(OwnNormal(PlainInitialiser), _type, _fields, _fieldValues, _fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	IN_PLACE_CREATABLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		if (type) {
			type.update_dependencies(phase, dep, ent, ctx);
		}
		for (auto field : fieldValues) {
			UPDATE_DEPS(field);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::PLAIN_INITIALISER; }

	useit Json to_json() const final;
};

} // namespace qat::ast

#endif
