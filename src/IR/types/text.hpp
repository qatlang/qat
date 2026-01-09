#ifndef QAT_IR_TYPES_TEXT_HPP
#define QAT_IR_TYPES_TEXT_HPP

#include "qat_type.hpp"

namespace qat::ir {

class Mod;

class TextType : public Type {
  private:
	bool isPack;

	static Vec<TextType*> allTextTypes;

  public:
	TextType(ir::Ctx* irCtx, bool isPacked = false);

	static TextType* get(ir::Ctx* irCtx, bool isPacked = false);

	static ir::PrerunValue* create_value(ir::Ctx* irCtx, ir::Mod* mod, String value);

	static String value_to_string(ir::PrerunValue* value);

	bool is_packed() const;

	TypeKind type_kind() const override;

	String to_string() const override;

	bool can_be_prerun() const final { return true; }

	bool can_be_prerun_generic() const final { return true; }

	bool is_type_sized() const final;

	bool has_simple_copy() const final { return true; }

	bool has_simple_move() const final { return true; }

	Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;
};

} // namespace qat::ir

#endif
