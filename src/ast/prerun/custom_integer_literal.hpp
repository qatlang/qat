#ifndef QAT_AST_PRERUN_CUSTOM_INTEGER_LITERAL_HPP
#define QAT_AST_PRERUN_CUSTOM_INTEGER_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../../utils/helpers.hpp"
#include "../expression.hpp"

namespace qat::ast {

class CustomIntegerLiteral final : public PrerunExpression, public TypeInferrable {
	String		value;
	Maybe<u32>	bitWidth;
	Maybe<u8>	radix;
	Maybe<bool> isUnsigned;

	Maybe<Identifier> suffix;

	static String radixDigits;
	static String radixDigitsUpper;

  public:
	CustomIntegerLiteral(String _value, Maybe<bool> _isUnsigned, Maybe<u32> _bitWidth, Maybe<u8> _radix,
						 Maybe<Identifier> _suffix, FileRange _fileRange)
		: PrerunExpression(std::move(_fileRange)), value(std::move(_value)), bitWidth(_bitWidth), radix(_radix),
		  isUnsigned(_isUnsigned), suffix(_suffix) {}

	useit static CustomIntegerLiteral* create(String _value, Maybe<bool> _isUnsigned, Maybe<u32> _bitWidth,
											  Maybe<u8> _radix, Maybe<Identifier> _suffix, FileRange _fileRange) {
		return std::construct_at(OwnNormal(CustomIntegerLiteral), _value, _isUnsigned, _bitWidth, _radix, _suffix,
								 _fileRange);
	}

	TYPE_INFERRABLE_FUNCTIONS

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
	}

	ir::PrerunValue* emit(EmitCtx* ctx) final;

	useit static String radixToString(u8 val);

	useit Json	 to_json() const final;
	useit String to_string() const final;

	useit NodeType nodeType() const final { return NodeType::CUSTOM_INTEGER_LITERAL; }
};

} // namespace qat::ast

#endif
