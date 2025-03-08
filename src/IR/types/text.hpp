#ifndef QAT_IR_TYPES_STRING_SLICE_HPP
#define QAT_IR_TYPES_STRING_SLICE_HPP

#include "qat_type.hpp"

namespace qat::ir {

class Mod;

class TextType : public Type {
  private:
	bool isPack;

  public:
	TextType(ir::Ctx* irCtx, bool isPacked = false);

	useit static TextType* get(ir::Ctx* irCtx, bool isPacked = false);

	useit static ir::PrerunValue* create_value(ir::Ctx* irCtx, ir::Mod* mod, String value);

	useit static String value_to_string(ir::PrerunValue* value);

	useit bool is_packed() const;

	useit TypeKind type_kind() const override;

	useit String to_string() const override;

	useit bool can_be_prerun() const final { return true; }

	useit bool can_be_prerun_generic() const final { return true; }

	useit bool is_type_sized() const final;

	useit bool is_trivially_copyable() const final { return true; }

	useit bool is_trivially_movable() const final { return true; }

	useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;
};

} // namespace qat::ir

#endif
