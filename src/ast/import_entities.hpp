#ifndef QAT_AST_IMPORT_ENTITIES_HPP
#define QAT_AST_IMPORT_ENTITIES_HPP

#include "../IR/context.hpp"
#include "../utils/file_range.hpp"
#include "./node.hpp"
#include "./node_type.hpp"

namespace qat::ast {

class ImportGroup {
	friend class ImportEntities;

  private:
	u32               relative;
	Vec<Identifier>   entity;
	Maybe<Identifier> alias;
	Vec<ImportGroup*> members;
	FileRangePtr      fileRange;

	mutable bool             isAlreadyImported = false;
	mutable ir::EntityState* entityState       = nullptr;

  public:
	ImportGroup(u32 _relative, Vec<Identifier> _entity, Maybe<Identifier> _alias, FileRangePtr _fileRange)
	    : relative(_relative), entity(std::move(_entity)), alias(std::move(_alias)), fileRange(std::move(_fileRange)) {}

	static ImportGroup* create(u32 relative, Vec<Identifier> parent, Maybe<Identifier> alias, FileRangePtr range) {
		return std::construct_at(OwnNormal(ImportGroup), relative, std::move(parent), std::move(alias),
		                         std::move(range));
	}

	void add_member(ImportGroup* mem);

	void extend_filerange(FileRangePtr end);

	void perform_import() const;

	bool has_members() const;

	bool is_all_imported() const;
};

class ImportEntities final : public IsEntity {
	Vec<ImportGroup*>     entities;
	Maybe<VisibilitySpec> visibSpec;

	mutable bool throwErrorsWhenUnfound = false;

  public:
	ImportEntities(Vec<ImportGroup*> _entities, Maybe<VisibilitySpec> _visibSpec, FileRangePtr _fileRange)
	    : IsEntity(_fileRange), entities(_entities), visibSpec(_visibSpec) {}

	static ImportEntities* create(Vec<ImportGroup*> _entities, Maybe<VisibilitySpec> _visibSpec,
	                              FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(ImportEntities), _entities, _visibSpec, _fileRange);
	}

	void create_entity(ir::Mod* mod, ir::Ctx* irCtx) final;

	void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) final;

	void do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) final;

	void handle_imports(ir::Mod* mod, ir::Ctx* irCtx) const;

	NodeType nodeType() const final { return NodeType::IMPORT_ENTITIES; }

	~ImportEntities() final;
};

} // namespace qat::ast

#endif
