#ifndef QAT_IR_GLOBAL_ENTITY_HPP
#define QAT_IR_GLOBAL_ENTITY_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./entity_overview.hpp"
#include "./value.hpp"

namespace qat::ir {

class Mod;

class PrerunGlobal : public PrerunValue, public EntityOverview {
	Identifier     name;
	VisibilityInfo visibility;
	Mod*           parent;

  public:
	PrerunGlobal(Mod* _parent, Identifier _name, Type* _type, llvm::Constant* _constant, VisibilityInfo _visibility,
	             FileRange _fileRange);

	useit static PrerunGlobal* create(Mod* _parent, Identifier _name, Type* _type, llvm::Constant* _constant,
	                                  VisibilityInfo _visibility, FileRange _fileRange) {
		return std::construct_at(OwnNormal(PrerunGlobal), _parent, std::move(_name), _type, _constant, _visibility,
		                         std::move(_fileRange));
	}

	useit Identifier get_name() const { return name; }
	useit String     get_full_name() const;
	useit ir::Mod*              get_parent() const { return parent; }
	useit VisibilityInfo const& get_visibility() const { return visibility; }
};

class GlobalEntity : public Value, public EntityOverview {
	Identifier             name;
	VisibilityInfo         visibility;
	Mod*                   parent;
	Maybe<llvm::Constant*> initialValue;

  public:
	GlobalEntity(Mod* _parent, Identifier _name, Type* _type, bool _is_variable, Maybe<llvm::Constant*> initialValue,
	             llvm::Value* _value, const VisibilityInfo& _visibility);

	useit static GlobalEntity* create(Mod* _parent, Identifier _name, Type* _type, bool _is_variable,
	                                  Maybe<llvm::Constant*> initialValue, llvm::Value* _value,
	                                  const VisibilityInfo& _visibility) {
		return std::construct_at(OwnNormal(GlobalEntity), _parent, _name, _type, _is_variable, initialValue, _value,
		                         _visibility);
	}

	~GlobalEntity() {
		if (ll && llvm::isa<llvm::GlobalVariable>(ll)) {
			std::destroy_at(llvm::cast<llvm::GlobalVariable>(ll));
		}
	}

	useit Identifier get_name() const;
	useit String     get_full_name() const;
	useit ir::Mod* get_module() const;
	useit bool     has_initial_value() const;
	useit llvm::Constant*       get_initial_value() const;
	useit const VisibilityInfo& get_visibility() const;
};

} // namespace qat::ir

#endif
