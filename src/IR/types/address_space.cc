#include "./address_space.hpp"
#include "../context.hpp"

namespace qat::ir {

Maybe<AddressSpace> AddressSpace::get_simplified_local_space(Ctx* irCtx) {
	if (irCtx->dataLayout.getProgramAddressSpace() == irCtx->dataLayout.getAllocaAddrSpace() &&
	    (irCtx->dataLayout.getProgramAddressSpace() == 0u)) {
		return None;
	} else {
		return AddressSpace::from_name("local");
	}
}

Maybe<AddressSpace> AddressSpace::get_simplified_global_space(Ctx* irCtx) {
	if (irCtx->dataLayout.getProgramAddressSpace() == irCtx->dataLayout.getDefaultGlobalsAddressSpace() &&
	    (irCtx->dataLayout.getProgramAddressSpace() == 0u)) {
		return None;
	} else {
		return AddressSpace::from_name("global");
	}
}

u32 AddressSpace::get_number(ir::Ctx* irCtx) const {
	if (name.empty()) {
		return value;
	}
	if (name == "global") {
		return irCtx->dataLayout.getDefaultGlobalsAddressSpace();
	} else if (name == "program") {
		return irCtx->dataLayout.getProgramAddressSpace();
	} else if (name == "local") {
		return irCtx->dataLayout.getAllocaAddrSpace();
	}
	return 0;
}

} // namespace qat::ir
