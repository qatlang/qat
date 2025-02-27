#ifndef QAT_AST_PRERUN_EXPRESSIONS_FLAG_INITIALISER_HPP
#define QAT_AST_PRERUN_EXPRESSIONS_FLAG_INITIALISER_HPP

#include "../expression.hpp"

namespace qat::ast {

class FlagInitialiser : public PrerunExpression, public TypeInferrable {
	Type*            type;
	Maybe<FileRange> specialRange;
	bool             isSpecialDefault;
	Vec<Identifier>  variants;

  public:
	FlagInitialiser(Type* _type, Maybe<FileRange> _specialRange, bool _isSpecialDefault, Vec<Identifier> _variants,
	                FileRange _range)
	    : PrerunExpression(std::move(_range)), type(_type), specialRange(_specialRange),
	      isSpecialDefault(_isSpecialDefault), variants(_variants) {}

	TYPE_INFERRABLE_FUNCTIONS

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::FLAG_INITIALISER; }

	useit String to_string() const final {}
};

} // namespace qat::ast

#endif
