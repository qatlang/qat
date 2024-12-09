#ifndef QAT_AST_EXPRESSIONS_ENTITY_HPP
#define QAT_AST_EXPRESSIONS_ENTITY_HPP

#include "../expression.hpp"

namespace qat::ast {

/**
 *  Entity represents either a variable or constant. The name of the
 * entity is mostly just an identifier, but it can be other values if the
 * entity is present in the global constant
 *
 */
class Entity final : public Expression {

  private:
	Vec<Identifier> names;
	u32             relative;

  public:
	Entity(u32 _relative, Vec<Identifier> _name, FileRange _fileRange)
	    : Expression(std::move(_fileRange)), names(std::move(_name)), relative(_relative) {}

	static Entity* create(u32 relative, Vec<Identifier> _name, FileRange _fileRange) {
		return std::construct_at(OwnNormal(Entity), relative, _name, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::Value* emit(EmitCtx* ctx);
	useit Json       to_json() const final;
	useit NodeType   nodeType() const final { return NodeType::ENTITY; }
};

} // namespace qat::ast

#endif
