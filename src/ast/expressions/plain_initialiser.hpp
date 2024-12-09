#ifndef QAT_AST_PLAIN_INITIALISER_HPP
#define QAT_AST_PLAIN_INITIALISER_HPP

#include "../expression.hpp"
#include "../types/named.hpp"

namespace qat::ast {

class PlainInitialiser final : public Expression, public LocalDeclCompatible, public TypeInferrable {
	friend class LocalDeclaration;

  private:
	Maybe<Type*>                 type;
	Vec<Pair<String, FileRange>> fields;
	Vec<u64>                     indices;
	Vec<Expression*>             fieldValues;

  public:
	PlainInitialiser(Maybe<Type*> _type, Vec<Pair<String, FileRange>> _fields, Vec<Expression*> _fieldValues,
	                 FileRange _fileRange)
	    : Expression(_fileRange), type(_type), fields(_fields), fieldValues(_fieldValues) {}

	useit static PlainInitialiser* create(Maybe<Type*> _type, Vec<Pair<String, FileRange>> _fields,
	                                      Vec<Expression*> _fieldValues, FileRange _fileRange) {
		return std::construct_at(OwnNormal(PlainInitialiser), _type, _fields, _fieldValues, _fileRange);
	}

	LOCAL_DECL_COMPATIBLE_FUNCTIONS
	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		if (type) {
			UPDATE_DEPS(type.value());
		}
		for (auto field : fieldValues) {
			UPDATE_DEPS(field);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) final;
	useit NodeType   nodeType() const final { return NodeType::PLAIN_INITIALISER; }
	useit Json       to_json() const final;
};

} // namespace qat::ast

#endif
