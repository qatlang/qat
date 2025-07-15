#ifndef QAT_AST_NONE_HPP
#define QAT_AST_NONE_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class NoneExpression final : public PrerunExpression, public TypeInferrable {
	friend class Assignment;
	friend class LocalDeclaration;
	Type*               type = nullptr;
	Maybe<FileRangePtr> isPacked;

  public:
	NoneExpression(Maybe<FileRangePtr> _isPacked, Type* _type, FileRangePtr _fileRange)
	    : PrerunExpression(std::move(_fileRange)), type(_type), isPacked(_isPacked) {}

	useit static NoneExpression* create(Maybe<FileRangePtr> isPacked, Type* _type, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(NoneExpression), isPacked, _type, _fileRange);
	}

	useit bool hasTypeSet() const { return type != nullptr; }

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		if (type) {
			UPDATE_DEPS(type);
		}
	}

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;
	useit Json             to_json() const final;
	useit String           to_string() const final;

	useit NodeType nodeType() const final { return NodeType::NONE; }
};

} // namespace qat::ast

#endif
