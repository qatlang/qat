#ifndef QAT_AST_PRERUN_EXPRESSIONS_FLAG_INITIALISER_HPP
#define QAT_AST_PRERUN_EXPRESSIONS_FLAG_INITIALISER_HPP

#include "../expression.hpp"
#include "../type_like.hpp"

namespace qat::ast {

class FlagInitialiser final : public PrerunExpression, public TypeInferrable {
	TypeLike            type;
	Maybe<FileRangePtr> specialRange;
	bool                isSpecialDefault;
	Vec<Identifier>     variants;

  public:
	FlagInitialiser(TypeLike _type, Maybe<FileRangePtr> _specialRange, bool _isSpecialDefault,
	                Vec<Identifier> _variants, FileRangePtr _range)
	    : PrerunExpression(std::move(_range)), type(_type), specialRange(_specialRange),
	      isSpecialDefault(_isSpecialDefault), variants(_variants) {}

	static FlagInitialiser* create(TypeLike type, Maybe<FileRangePtr> specialRange, bool isSpecialDefault,
	                               Vec<Identifier> variants, FileRangePtr range) {
		return std::construct_at(OwnNormal(FlagInitialiser), type, std::move(specialRange), isSpecialDefault,
		                         std::move(variants), std::move(range));
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	ir::PrerunValue* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::FLAG_INITIALISER; }

	String to_string() const final;
};

} // namespace qat::ast

#endif
