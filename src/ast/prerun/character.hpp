#ifndef QAT_AST_PRERUN_CHARACTER_HPP
#define QAT_AST_PRERUN_CHARACTER_HPP

#include "../expression.hpp"

#include <helpers/array.hpp>

namespace qat::ast {

class Character final : public PrerunExpression {
	bool         isByte;
	Array<u8, 4> bytes;

  public:
	Character(bool _isByte, Array<u8, 4> _character, FileRangePtr _fileRange)
	    : PrerunExpression(std::move(_fileRange)), isByte(_isByte), bytes(_character) {}

	static Character* create_char(Array<u8, 4> character, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(Character), false, character, std::move(fileRange));
	}

	static Character* create_byte(u8 character, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(Character), true, Array<u8, 4>{character, 0, 0, 0}, std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	ir::PrerunValue* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::CHARACTER; }

	String to_string() const final;
};

} // namespace qat::ast

#endif
