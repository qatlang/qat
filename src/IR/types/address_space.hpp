#ifndef QAT_IR_TYPES_ADDRESS_SPACE_HPP
#define QAT_IR_TYPES_ADDRESS_SPACE_HPP

#include "../../utils/helpers.hpp"
#include "../../utils/macros.hpp"

namespace qat::ir {

class Ctx;

struct AddressSpace {
	String name;
	u32    value;

	useit static AddressSpace from_value(u32 _value) { return AddressSpace{.name = "", .value = _value}; }

	useit static AddressSpace from_name(String _name) { return AddressSpace{.name = std::move(_name), .value = 0}; }

	useit static Maybe<AddressSpace> get_simplified_local_space(Ctx* irCtx);

	useit static Maybe<AddressSpace> get_simplified_global_space(Ctx* irCtx);

	useit static bool compare(Maybe<AddressSpace> const& first, Maybe<AddressSpace> const& second) {
		return (first.has_value() == second.has_value()) &&
		       (first.has_value() ? ((first->name == second->name) && (first->value == second->value)) : true);
	}

	useit String to_string() const {
		if (name.empty()) {
			return "of(" + std::to_string(value) + ")";
		} else {
			return "of:" + name;
		}
	}

	useit u32 get_number(ir::Ctx* irCtx) const;
};

} // namespace qat::ir

#endif
