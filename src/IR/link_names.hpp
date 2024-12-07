#ifndef QAT_IR_LINK_NAMES_HPP
#define QAT_IR_LINK_NAMES_HPP

#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"

namespace qat::ir {
class Mod;
}

namespace qat {

enum class LinkUnitType {
	lib,
	type,
	function,
	method,
	variationMethod,
	staticFunction,
	value_method,
	constructor,
	fromConvertor,
	toConvertor,
	defaultConstructor,
	copyConstructor,
	copyAssignment,
	moveConstructor,
	moveAssignment,
	normalBinaryOperator,
	variationBinaryOperator,
	unaryOperator,
	destructor,
	staticField,
	choice,
	mix,
	genericList,
	genericPrerunValue,
	genericTypeValue,
	region,
	skill,
	doType,
	doSkill,
	name,
	typeName,
	global,
	toggle,
};

class LinkNameUnit;

class LinkNames {
  public:
	Vec<LinkNameUnit> units;
	Maybe<String>	  linkAlias;
	Maybe<String>	  foreignID;
	ir::Mod*		  parentMod;

	LinkNames();
	LinkNames(Vec<LinkNameUnit> _units, Maybe<String> _foreignID, ir::Mod* _mod);

	void			setLinkAlias(Maybe<String> _linkAlias);
	void			addUnit(LinkNameUnit unit, Maybe<String> foreignID);
	useit LinkNames newWith(LinkNameUnit unit, Maybe<String> foreignID);
	useit String	toName() const;
};

class LinkNameUnit {
  public:
	String		   name;
	LinkUnitType   unitType;
	Vec<LinkNames> subNames;

	LinkNameUnit(String _name, LinkUnitType _namingType, Vec<LinkNames> subNames = {});
};

} // namespace qat

#endif
