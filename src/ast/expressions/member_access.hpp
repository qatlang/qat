#ifndef QAT_AST_EXPRESSIONS_MEMBER_ACCESS_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_ACCESS_HPP

#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

class MemberAccess final : public Expression {
	Expression*                     instance;
	bool                            isExpSelf = false;
	Maybe<Pair<bool, FileRangePtr>> isVarRange;
	Identifier                      name;

  public:
	MemberAccess(Expression* _instance, bool _isExpSelf, Maybe<Pair<bool, FileRangePtr>> _isVarRange, Identifier _name,
	             FileRangePtr _fileRange)
	    : Expression(std::move(_fileRange)), instance(_instance), isExpSelf(_isExpSelf),
	      isVarRange(std::move(_isVarRange)), name(std::move(_name)) {}

	useit static MemberAccess* create(Expression* _instance, bool isExpSelf,
	                                  Maybe<Pair<bool, FileRangePtr>> _isVarRange, Identifier _name,
	                                  FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(MemberAccess), _instance, isExpSelf, std::move(_isVarRange), _name,
		                         _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(instance);
		for (auto mod : ir::Mod::allModules) {
			for (auto modEnt : mod->entityEntries) {
				if (modEnt->type == ir::EntityType::defaultDoneSkill) {
					if (modEnt->has_child(name.value)) {
						ent->addDependency(ir::EntityDependency{modEnt, ir::DependType::partial, phase});
					}
				} else if (modEnt->type == ir::EntityType::structType) {
					if (modEnt->has_child(name.value)) {
						ent->addDependency(ir::EntityDependency{modEnt, ir::DependType::childrenPartial, phase});
					}
				}
			}
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) override;
	useit Json       to_json() const override;

	useit NodeType nodeType() const override { return NodeType::MEMBER_ACCESS; }
};

} // namespace qat::ast

#endif
