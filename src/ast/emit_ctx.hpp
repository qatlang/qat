#ifndef QAT_AST_EMIT_CTX_HPP
#define QAT_AST_EMIT_CTX_HPP

#include "../IR/prerun_function.hpp"
#include "../IR/qat_module.hpp"

namespace qat::ast {

struct VisibilitySpec;

enum class LoopType {
	TO_COUNT,
	WHILE,
	DO_WHILE,
	OVER,
	INFINITE,
};

struct LoopInfo {
	Maybe<Identifier> name;
	Maybe<Identifier> secondaryName;
	ir::Block*        mainBlock;
	ir::Block*        condBlock;
	ir::Block*        restBlock;
	ir::LocalValue*   index;
	LoopType          type;

	LoopInfo(Maybe<Identifier> _name, ir::Block* _mainB, ir::Block* _condB, ir::Block* _restB, ir::LocalValue* _index,
	         LoopType _type)
	    : name(_name), mainBlock(_mainB), condBlock(_condB), restBlock(_restB), index(_index), type(_type) {}

	void set_secondary_name(Identifier other) { secondaryName = std::move(other); }

	useit bool isTimes() const { return type == LoopType::TO_COUNT; }
};

enum class BreakableType {
	loop,
	match,
};

struct Breakable {
	Maybe<Identifier> tag;
	ir::Block*        restBlock;
	ir::Block*        trueBlock;
	BreakableType     type;

	Breakable(BreakableType _type, Maybe<Identifier> _tag, ir::Block* _restBlock, ir::Block* _trueBlock)
	    : tag(std::move(_tag)), restBlock(_restBlock), trueBlock(_trueBlock), type(_type) {}

	String type_to_string() const {
		switch (type) {
			case BreakableType::loop:
				return "loop";
			case BreakableType::match:
				return "match";
		}
	}
};

struct EmitCtx {
	ir::Ctx* irCtx = nullptr;
	ir::Mod* mod   = nullptr;

	ir::Skill*        skill;
	ir::MethodParent* memberParent;
	ir::OpaqueType*   parentOpaque;
	ir::Function*     fn;

	ir::PrerunCallState* prerunCallState;

	Vec<LoopInfo>  loopsInfo;
	Vec<Breakable> breakables;

	EmitCtx(ir::Ctx* _irCtx, ir::Mod* _mod)
	    : irCtx(_irCtx), mod(_mod), skill(nullptr), memberParent(nullptr), parentOpaque(nullptr), fn(nullptr),
	      prerunCallState(nullptr) {}

	useit static EmitCtx* get(ir::Ctx* _irCtx, ir::Mod* _mod) {
		return std::construct_at(OwnNormal(EmitCtx), _irCtx, _mod);
	}

	EmitCtx* with_member_parent(ir::MethodParent* _memberParent) {
		memberParent = _memberParent;
		return this;
	}

	EmitCtx* with_function(ir::Function* _fn) {
		fn = _fn;
		return this;
	}

	EmitCtx* with_prerun_call_state(ir::PrerunCallState* _preFn) {
		prerunCallState = _preFn;
		return this;
	}

	EmitCtx* with_opaque_parent(ir::OpaqueType* _opq) {
		parentOpaque = _opq;
		return this;
	}

	useit AccessInfo get_access_info() const;

	useit bool has_member_parent() const { return memberParent != nullptr; }
	useit ir::MethodParent* get_member_parent() const { return memberParent; }

	useit bool has_fn() const { return fn != nullptr; }
	useit ir::Function* get_fn() const { return fn; }

	useit bool has_pre_call_state() const { return prerunCallState != nullptr; }
	useit ir::PrerunCallState* get_pre_call_state() const { return prerunCallState; }

	useit bool has_opaque_parent() const { return parentOpaque != nullptr; }
	useit ir::OpaqueType* get_opaque_parent() const { return parentOpaque; }

	useit bool has_skill() const { return skill != nullptr; }
	useit ir::Skill* get_skill() const { return skill; }

	useit String color(String const& message) const;

	void genericNameCheck(String const& name, FileRange const& range);

	void name_check_in_module(const Identifier& name, const String& entityType, Maybe<String> genericID = None,
	                          Maybe<String> opaqueID = None);

	void Error(const String& message, Maybe<FileRange> fileRange, Maybe<Pair<String, FileRange>> pointTo = None);

	VisibilityInfo get_visibility_info(Maybe<VisibilitySpec> spec);
};

} // namespace qat::ast

#endif
