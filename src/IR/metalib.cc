#include "./metalib.hpp"
#include "../ast/meta/intrinsic.hpp"
#include "./qat_module.hpp"
#include "./types/choice.hpp"
#include "./types/unsigned.hpp"

#define INTRINSIC_CHOICE_NAME "Intrinsics"

namespace qat::ir {

Mod* MetaLib::lib = nullptr;

void MetaLib::create(ir::Ctx* irCtx) {
	if (lib == nullptr) {
		lib = ir::Mod::create_root_lib(nullptr, "", "", Identifier::named("meta"), {}, VisibilityInfo::pub(), irCtx);
	}
}

ChoiceType* MetaLib::get_intrinsic_id(Ctx* irCtx) {
	create(irCtx);
	if (lib->has_choice_type(INTRINSIC_CHOICE_NAME, AccessInfo::get_privileged())) {
		return lib->get_choice_type(INTRINSIC_CHOICE_NAME, AccessInfo::get_privileged());
	}
	return ChoiceType::create(Identifier::named(INTRINSIC_CHOICE_NAME), lib, false,
	                          {
	                              {Identifier::named("matrix_multiply")},
	                              {Identifier::named("matrix_transpose")},
	                              {Identifier::named("matrix_column_major_load")},
	                              {Identifier::named("matrix_column_major_store")},
	                              {Identifier::named("read_cycle_counter")},
	                              {Identifier::named("read_steady_counter")},
	                              {Identifier::named("give_address")},
	                              {Identifier::named("caller_give_address")},
	                              {Identifier::named("thread_pointer")},
	                          },
	                          None, ir::UnsignedType::create(sizeof(ast::IntrinsicID), irCtx), true, None,
	                          VisibilityInfo::pub(), irCtx, FileRange::null, None);
}

} // namespace qat::ir
