#ifndef QAT_AST_BRING_ENTITIES_HPP
#define QAT_AST_BRING_ENTITIES_HPP

#include "../IR/context.hpp"
#include "../utils/file_range.hpp"
#include "./node.hpp"
#include "./node_type.hpp"

namespace qat::ast {

class BroughtGroup {
	friend class BringEntities;

  private:
	u32                relative;
	Vec<Identifier>    entity;
	Maybe<Identifier>  alias;
	Vec<BroughtGroup*> members;
	FileRangePtr       fileRange;

	mutable bool             isAlreadyBrought = false;
	mutable ir::EntityState* entityState      = nullptr;

  public:
	BroughtGroup(u32 _relative, Vec<Identifier> _entity, Maybe<Identifier> _alias, FileRangePtr _fileRange)
	    : relative(_relative), entity(std::move(_entity)), alias(std::move(_alias)), fileRange(std::move(_fileRange)) {}

	useit static BroughtGroup* create(u32 relative, Vec<Identifier> parent, Maybe<Identifier> alias,
	                                  FileRangePtr range) {
		return std::construct_at(OwnNormal(BroughtGroup), relative, std::move(parent), std::move(alias),
		                         std::move(range));
	}

	void add_member(BroughtGroup* mem);
	void extend_filerange(FileRangePtr end);
	void bring() const;

	useit bool has_members() const;
	useit bool is_all_brought() const;
	useit Json to_json() const;
};

class BringEntities final : public IsEntity {
	Vec<BroughtGroup*>    entities;
	Maybe<VisibilitySpec> visibSpec;

	mutable bool throwErrorsWhenUnfound = false;

  public:
	BringEntities(Vec<BroughtGroup*> _entities, Maybe<VisibilitySpec> _visibSpec, FileRangePtr _fileRange)
	    : IsEntity(_fileRange), entities(_entities), visibSpec(_visibSpec) {}

	useit static BringEntities* create(Vec<BroughtGroup*> _entities, Maybe<VisibilitySpec> _visibSpec,
	                                   FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(BringEntities), _entities, _visibSpec, _fileRange);
	}

	void create_entity(ir::Mod* mod, ir::Ctx* irCtx) final;
	void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) final;
	void do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) final;

	void handle_brings(ir::Mod* mod, ir::Ctx* irCtx) const;

	useit Json to_json() const final;

	useit NodeType nodeType() const final { return NodeType::BRING_ENTITIES; }

	~BringEntities() final;
};

} // namespace qat::ast

#endif
