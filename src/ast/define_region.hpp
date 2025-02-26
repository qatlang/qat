#ifndef QAT_AST_DEFINE_REGION_HPP
#define QAT_AST_DEFINE_REGION_HPP

#include "node.hpp"
#include "node_type.hpp"

namespace qat::ast {

class DefineRegion : public IsEntity {
  private:
	Identifier            name;
	PrerunExpression*     blockSize;
	Maybe<VisibilitySpec> visibSpec;

  public:
	DefineRegion(Identifier _name, PrerunExpression* _blockSize, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
	    : IsEntity(_fileRange), name(_name), blockSize(_blockSize), visibSpec(_visibSpec) {}

	useit static DefineRegion* create(Identifier name, PrerunExpression* blockSize, Maybe<VisibilitySpec> visibSpec,
	                                  FileRange fileRange) {
		return std::construct_at(OwnNormal(DefineRegion), name, blockSize, visibSpec, fileRange);
	}

	void create_entity(ir::Mod* mod, ir::Ctx* irCtx) final;
	void update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) final;
	void do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) final;

	useit Json     to_json() const final;
	useit NodeType nodeType() const final { return NodeType::DEFINE_REGION; }
};

} // namespace qat::ast

#endif
