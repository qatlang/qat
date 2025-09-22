#include "./address_space.hpp"

namespace qat::ir {

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
