#ifndef QAT_IR_PRERUN_FUNCTION_HPP
#define QAT_IR_PRERUN_FUNCTION_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./types/function.hpp"
#include "./types/qat_type.hpp"
#include "entity_overview.hpp"
#include "value.hpp"

namespace qat::ast {
class PrerunSentence;
class PrerunLoopTo;
class PrerunBreak;
class PrerunContinue;
} // namespace qat::ast

namespace qat::ir {

class PrerunFunction;

class PrerunLocal {
	Identifier   name;
	Type*        type = nullptr;
	bool         isVar;
	PrerunValue* value = nullptr;

	PrerunLocal(Identifier _name, Type* _type, bool _isVar, PrerunValue* _initialVal)
	    : name(_name), type(_type), isVar(_isVar), value(_initialVal) {}

  public:
	useit static PrerunLocal* get(Identifier _name, Type* _type, bool _isVar, PrerunValue* _initialVal) {
		return new PrerunLocal(_name, _type, _isVar, _initialVal);
	}

	useit Identifier   get_name() const { return name; }
	useit Type*        get_type() const { return type; }
	useit bool         is_variable() const { return isVar; }
	useit PrerunValue* get_value() const { return value; }

	void change_value(PrerunValue* other) { value = other; }
};

class PreBlock {
	PrerunCallState*  callState;
	Vec<PrerunLocal*> locals;
	PreBlock*         firstChild = nullptr;
	PreBlock*         parent     = nullptr;
	PreBlock*         previous   = nullptr;
	PreBlock*         next       = nullptr;

  public:
	PreBlock(PrerunCallState* _callState, PreBlock* _parent);

	useit static PreBlock* create(PrerunCallState* callState, PreBlock* parent = nullptr) {
		return std::construct_at(OwnNormal(PreBlock), callState, parent);
	}

	useit static PreBlock* create_next_to(PreBlock* previous) {
		auto res = std::construct_at(OwnNormal(PreBlock), previous->callState, previous->parent);
		res->set_previous(previous);
		return res;
	}

	~PreBlock() {
		for (auto loc : locals) {
			std::destroy_at(loc);
		}
		if (next != nullptr) {
			std::destroy_at(next);
		}
	}

	void make_active();

	useit bool has_previous() const { return previous != nullptr; }
	useit bool has_next() const { return next != nullptr; }
	useit bool has_parent() const { return parent != nullptr; }
	useit bool has_local(String const& name) {
		for (auto loc : locals) {
			if (loc->get_name().value == name) {
				return true;
			}
		}
		if (previous != nullptr) {
			auto pRes = previous->has_local(name);
			if (pRes) {
				return pRes;
			}
		}
		if (parent != nullptr) {
			auto pRes = parent->has_local(name);
			if (pRes) {
				return pRes;
			}
		}
		return false;
	}

	useit PrerunCallState* get_call_state() const { return callState; }

	useit PrerunLocal* get_local(String const& name) {
		for (auto loc : locals) {
			if (loc->get_name().value == name) {
				return loc;
			}
		}
		return nullptr;
	}

	void set_previous(PreBlock* _prev) {
		previous       = _prev;
		previous->next = this;
	}
};

enum class PreLoopKind {
	TO,
	IF,
	IN,
	INFINITE,
};

struct PreLoopInfo {
	PreLoopKind       kind;
	Maybe<Identifier> tag;

	useit String kind_to_string() const {
		switch (kind) {
			case PreLoopKind::TO:
				return "loop to";
			case PreLoopKind::IF:
				return "loop if";
			case PreLoopKind::IN:
				return "loop in";
			case PreLoopKind::INFINITE:
				return "loop";
		}
	}
};

class PrerunCallState {
	friend class PrerunFunction;
	friend class ast::PrerunLoopTo;
	friend class ast::PrerunBreak;
	friend class ast::PrerunContinue;
	friend class PreBlock;

	PrerunFunction*   function = nullptr;
	Vec<PrerunValue*> argumentValues;
	PreBlock*         rootBlock;
	PreBlock*         activeBlock;
	Vec<PreLoopInfo>  loopsInfo;

  public:
	PrerunCallState(PrerunFunction* _function, Vec<PrerunValue*> _argVals)
	    : function(_function), argumentValues(_argVals), rootBlock(PreBlock::create(this, nullptr)),
	      activeBlock(rootBlock) {}

	~PrerunCallState() { std::destroy_at(rootBlock); }

	useit static PrerunCallState* get(PrerunFunction* fun, Vec<PrerunValue*> argVals) {
		return std::construct_at(OwnNormal(PrerunCallState), fun, argVals);
	}

	useit PreBlock*       get_block() const { return activeBlock; }
	useit PrerunFunction* get_function() const { return function; }
	useit bool            has_arg_with_name(String const& name);
	useit PrerunValue*    get_arg_value_for(String const& name);
};

class PrerunFunction : public PrerunValue, public EntityOverview {
	friend class PrerunCallState;
	Identifier         name;
	Type*              returnType;
	Vec<ArgumentType*> argTypes;
	Mod*               parent;
	VisibilityInfo     visibility;

	Pair<Vec<ast::PrerunSentence*>, FileRange> sentences;

  public:
	PrerunFunction(Mod* _parent, Identifier _name, Type* _retTy, Vec<ArgumentType*> _argTys,
	               Pair<Vec<ast::PrerunSentence*>, FileRange> _sentences, VisibilityInfo visib, llvm::LLVMContext& ctx);

	useit static PrerunFunction* create(Mod* parent, Identifier name, Type* returnTy, Vec<ArgumentType*> argTypes,
	                                    Pair<Vec<ast::PrerunSentence*>, FileRange> sentences, VisibilityInfo visibility,
	                                    llvm::LLVMContext& ctx) {
		return std::construct_at(OwnNormal(PrerunFunction), parent, std::move(name), returnTy, std::move(argTypes),
		                         std::move(sentences), visibility, ctx);
	}

	void update_overview() final;

	useit Identifier    get_name() const { return name; }
	useit String        get_full_name() const;
	useit Type*         get_return_type() const { return returnType; }
	useit ArgumentType* get_argument_type_at(usize index) { return argTypes[index]; }
	useit Mod*          get_module() const { return parent; }

	useit VisibilityInfo const& get_visibility() const { return visibility; }

	PrerunValue* call_prerun(Vec<PrerunValue*> arguments, Ctx* irCtx, FileRange fileRange);
};

} // namespace qat::ir

#endif
