#ifndef QAT_IR_GENERIC_VARIANT_HPP
#define QAT_IR_GENERIC_VARIANT_HPP

#include "../show.hpp"
#include "../utils/helpers.hpp"
#include "../utils/macros.hpp"
#include "./generics.hpp"
#include "./value.hpp"
#include "types/qat_type.hpp"

namespace qat::ir {

template <typename T> class GenericVariant {
  private:
	T*						entity;
	Vec<ir::GenericToFill*> genericTypes;

  public:
	GenericVariant(T* _entity, Vec<ir::GenericToFill*> _types) : entity(_entity), genericTypes(std::move(_types)) {}

	useit bool check(ir::Ctx* irCtx, std::function<void(const String&, const FileRange&)> errorFn,
					 Vec<GenericToFill*> dest) const {
		if (genericTypes.size() != dest.size()) {
			return false;
		} else {
			for (usize i = 0; i < genericTypes.size(); i++) {
				auto* genTy = genericTypes.at(i);
				if (genTy->is_type()) {
					if (dest.at(i)->is_type()) {
						if (!genTy->as_type()->is_same(dest.at(i)->as_type())) {
							return false;
						}
					} else {
						auto* preVal = dest.at(i)->as_prerun();
						if (preVal->get_ir_type()->is_typed()) {
							if (!genTy->as_type()->is_same(preVal->get_ir_type()->as_typed()->get_subtype())) {
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
								if (!genExp->get_ir_type()->as_typed()->get_subtype()->is_same(
										destExp->get_ir_type()->as_typed()->get_subtype())) {
									return false;
								}
							} else {
								return false;
							}
						} else {
							if (genExp->get_ir_type()->is_same(destExp->get_ir_type())) {
								auto eqRes = genExp->get_ir_type()->equality_of(irCtx, genExp, destExp);
								if (eqRes.has_value()) {
									if (!eqRes.value()) {
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
							if (!dest.at(i)->as_type()->is_same(
									genTy->as_prerun()->get_ir_type()->as_typed()->get_subtype())) {
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
