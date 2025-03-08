#ifndef QAT_AST_PRERUN_EXPRESSIONS_FLAG_INITIALISER_HPP
#define QAT_AST_PRERUN_EXPRESSIONS_FLAG_INITIALISER_HPP

#include "../expression.hpp"
#include "../type_like.hpp"

namespace qat::ast {

class FlagInitialiser final : public PrerunExpression, public TypeInferrable {
	TypeLike         type;
	Maybe<FileRange> specialRange;
	bool             isSpecialDefault;
	Vec<Identifier>  variants;

  public:
	FlagInitialiser(TypeLike _type, Maybe<FileRange> _specialRange, bool _isSpecialDefault, Vec<Identifier> _variants,
	                FileRange _range)
	    : PrerunExpression(std::move(_range)), type(_type), specialRange(_specialRange),
	      isSpecialDefault(_isSpecialDefault), variants(_variants) {}

	useit static FlagInitialiser* create(TypeLike type, Maybe<FileRange> specialRange, bool isSpecialDefault,
	                                     Vec<Identifier> variants, FileRange range) {
		return std::construct_at(OwnNormal(FlagInitialiser), type, std::move(specialRange), isSpecialDefault,
		                         std::move(variants), std::move(range));
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::FLAG_INITIALISER; }

	useit Json to_json() const final;

	useit String to_string() const final;
};

} // namespace qat::ast

#endif
