#ifndef QAT_IR_BROUGHT_HPP
#define QAT_IR_BROUGHT_HPP

#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include <functional>
#include <optional>

namespace qat::ir {

template <class T> class Brought {
	friend ir::Mod;
	template <typename E>
	friend bool matchBroughtEntity(Brought<E> brought, String candName, Maybe<AccessInfo> reqInfo);

	Maybe<Identifier> name;
	T*                entity;
	VisibilityInfo    visibility;

  public:
	Brought(Identifier _name, T* _entity, const VisibilityInfo& _visibility)
	    : name(_name), entity(_entity), visibility(_visibility) {}

	Brought(T* _entity, const VisibilityInfo& _visibility) : entity(_entity), visibility(_visibility) {}

	useit Identifier get_name() const { return name.value_or(Identifier("", {""})); }

	useit bool is_named() const { return name.has_value(); }

	useit T* get() const { return entity; }

	useit const VisibilityInfo& get_visibility() const { return visibility; }
};

template <typename T> useit bool matchBroughtEntity(Brought<T> brought, String candName, Maybe<AccessInfo> reqInfo) {
	if (brought.is_named()) {
		return (brought.name.value().value == candName) && brought.visibility.is_accessible(reqInfo) &&
		       brought.entity->get_visibility().is_accessible(reqInfo);
	} else {
		return (brought.entity->get_name().value == candName) && brought.visibility.is_accessible(reqInfo) &&
		       brought.entity->get_visibility().is_accessible(reqInfo);
	}
}

} // namespace qat::ir

#endif
