#ifndef QAT_AST_ASSEMBLY_BLOCK_HPP
#define QAT_AST_ASSEMBLY_BLOCK_HPP

#include "./node.hpp"

namespace qat::ast {

class AssemblyBlock final : public IsEntity {
	PrerunExpression* content;

	PrerunExpression* defineChecker;

  public:
	AssemblyBlock(PrerunExpression* _content, PrerunExpression* _defineChecker, FileRange _fileRange)
	    : IsEntity(std::move(_fileRange)), content(std::move(_content)), defineChecker(_defineChecker) {}

	useit static AssemblyBlock* create(PrerunExpression* content, PrerunExpression* defineChecker,
	                                   FileRange fileRange) {
		return std::construct_at(OwnNormal(AssemblyBlock), content, defineChecker, std::move(fileRange));
	}

	void create_entity(ir::Mod* mod, ir::Ctx* irCtx) final;

	void update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) final;

	void do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) final;

	useit Json to_json() const final;

	useit NodeType nodeType() const final { return NodeType::ASSEMBLY_BLOCK; }
};

} // namespace qat::ast

#endif
