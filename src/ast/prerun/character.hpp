#ifndef QAT_AST_PRERUN_CHARACTER_HPP
#define QAT_AST_PRERUN_CHARACTER_HPP

#include "../expression.hpp"

namespace qat::ast {

class Character final : public PrerunExpression {
	bool              isByte;
	std::array<u8, 4> bytes;

  public:
	Character(bool _isByte, std::array<u8, 4> _character, FileRangePtr _fileRange)
	    : PrerunExpression(std::move(_fileRange)), isByte(_isByte), bytes(_character) {}

	useit static Character* create_char(std::array<u8, 4> character, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(Character), false, character, std::move(fileRange));
	}

	useit static Character* create_byte(u8 character, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(Character), true, std::array<u8, 4>{character, 0, 0, 0},
		                         std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::CHARACTER; }

	useit String to_string() const final;

	useit Json to_json() const final;
};

} // namespace qat::ast

#endif
