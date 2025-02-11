#ifndef QAT_AST_DO_SKILL_HPP
#define QAT_AST_DO_SKILL_HPP

#include "./member_parent_like.hpp"
#include "./node.hpp"
#include "./skill_entity.hpp"
#include "./types/qat_type.hpp"

namespace qat::ast {

class DoSkill final : public IsEntity, public MemberParentLike {
	bool               isDefaultSkill;
	Maybe<SkillEntity> name;
	ast::Type*         targetType;

	mutable ir::DoneSkill*    doneSkill = nullptr;
	mutable ir::MethodParent* parent    = nullptr;

  public:
	DoSkill(bool _isDef, Maybe<SkillEntity> _name, ast::Type* _targetType, FileRange _fileRange)
	    : IsEntity(_fileRange), isDefaultSkill(_isDef), name(_name), targetType(_targetType) {}

	useit static DoSkill* create(bool _isDef, Maybe<SkillEntity> _name, ast::Type* _targetType, FileRange _fileRange) {
		return std::construct_at(OwnNormal(DoSkill), _isDef, _name, _targetType, _fileRange);
	}

	void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;
	void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) final;
	void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) final;

	void define_done_skill(ir::Mod* mod, ir::Ctx* irCtx);
	void define_types(ir::DoneSkill* skillImp, ir::Mod* mod, ir::Ctx* irCtx);
	void define_members(ir::Ctx* irCtx);
	void emit_members(ir::Ctx* irCtx);

	useit bool     is_done_skill() const final { return true; }
	useit DoSkill* as_done_skill() final { return this; }

	Json     to_json() const final;
	NodeType nodeType() const final { return NodeType::DO_SKILL; }
};

} // namespace qat::ast

#endif
