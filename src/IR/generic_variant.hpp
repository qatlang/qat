#ifndef QAT_IR_GENERIC_VARIANT_HPP
#define QAT_IR_GENERIC_VARIANT_HPP

#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"
#include "./generics.hpp"
#include "./type_id.hpp"
#include "./types/qat_type.hpp"
#include "./value.hpp"

namespace qat::ir {

template <typename T> class GenericVariant {
  private:
	T*                      entity;
	Vec<ir::GenericToFill*> genericTypes;

  public:
	GenericVariant(T* _entity, Vec<ir::GenericToFill*> _types) : entity(_entity), genericTypes(std::move(_types)) {}

	~GenericVariant() = default;

	void clear_fill_types() {
		for (auto* fill : genericTypes) {
			std::destroy_at(fill);
		}
		genericTypes.clear();
	}

	useit bool check(ir::Ctx* irCtx, std::function<void(const String&, const FileRange&)> errorFn,
	                 Vec<GenericToFill*> dest) const {
		if (genericTypes.size() != dest.size()) {
			return false;
		} else {
			for (usize i = 0; i < genericTypes.size(); i++) {
				auto* genTy = genericTypes.at(i);
				if (genTy->is_type()) {
					if (dest.at(i)->is_type()) {
						if (not genTy->as_type()->is_same(dest.at(i)->as_type())) {
							return false;
						}
					} else {
						auto* preVal = dest.at(i)->as_prerun();
						if (preVal->get_ir_type()->is_typed()) {
							if (not genTy->as_type()->is_same(TypeInfo::get_for(preVal->get_llvm_constant())->type)) {
								return false;
							}
						} else {
							return false;
						}
					}
				} else if (genTy->is_prerun()) {
					if (dest.at(i)->is_prerun()) {
						auto* genExp  = genTy->as_prerun();
						auto* destExp = dest.at(i)->as_prerun();
						if (genExp->get_ir_type()->is_typed()) {
							if (destExp->get_ir_type()->is_typed()) {
								if (not TypeInfo::get_for(genExp->get_llvm_constant())
								            ->type->is_same(TypeInfo::get_for(destExp->get_llvm_constant())->type)) {
									return false;
								}
							} else {
								return false;
							}
						} else {
							if (genExp->get_ir_type()->is_same(destExp->get_ir_type())) {
								auto eqRes = genExp->get_ir_type()->equality_of(irCtx, genExp, destExp);
								if (eqRes.has_value()) {
									if (not eqRes.value()) {
										return false;
									}
								} else {
									return false;
								}
							} else {
								return false;
							}
						}
					} else {
						if (genTy->as_prerun()->get_ir_type()->is_typed()) {
							if (not dest.at(i)->as_type()->is_same(
							        TypeInfo::get_for(genTy->as_prerun()->get_llvm_constant())->type)) {
								return false;
							}
						} else {
							return false;
						}
					}
				} else {
					errorFn("Invalid generic kind", genTy->get_range());
				}
			}
			return true;
		}
	}
	useit T* get() { return entity; }
};

} // namespace qat::ir

#endif
