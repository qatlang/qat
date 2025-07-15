#ifndef QAT_AST_ASSEMBLY_BLOCK_HPP
#define QAT_AST_ASSEMBLY_BLOCK_HPP

#include "./node.hpp"

namespace qat::ast {

class AssemblyBlock final : public IsEntity {
	PrerunExpression* content;
	PrerunExpression* defineChecker;

  public:
	AssemblyBlock(PrerunExpression* _content, PrerunExpression* _defineChecker, FileRangePtr _fileRange)
	    : IsEntity(_fileRange), content(std::move(_content)), defineChecker(_defineChecker) {
		SHOW("Created ast::AssemblyBlock");
	}

	useit static AssemblyBlock* create(PrerunExpression* content, PrerunExpression* defineChecker,
	                                   FileRangePtr fileRange) {
		SHOW("Calling construct_at")
		return std::construct_at(OwnNormal(AssemblyBlock), content, defineChecker, fileRange);
	}

	void create_entity(ir::Mod* mod, ir::Ctx* irCtx) final;

	void update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) final;

	void do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) final;

	useit Json to_json() const final;

	useit NodeType nodeType() const final { return NodeType::ASSEMBLY_BLOCK; }
};

} // namespace qat::ast

#endif
