#ifndef QAT_IR_TYPES_UNSIGNED_HPP
#define QAT_IR_TYPES_UNSIGNED_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::ir {

// Unsigned integer datatype in the language
class UnsignedType : public Type {
private:
  u64  bitWidth;
  bool isTypeBool;

  ir::Ctx* irCtx;

  UnsignedType(u64 _bitWidth, ir::Ctx* irCtx, bool _isBool);

public:
  useit static UnsignedType* get(u64 bits, ir::Ctx* llctx);
  useit static UnsignedType* getBool(ir::Ctx* llctx);
  useit inline u64           getBitwidth() const { return bitWidth; }
  useit inline bool          isBitWidth(u64 width) const { return bitWidth == width; }
  useit inline bool          isBoolean() const { return isTypeBool; }
  useit inline TypeKind      type_kind() const final { return TypeKind::unsignedInteger; }
  useit inline String        to_string() const final { return isTypeBool ? "bool" : ("u" + std::to_string(bitWidth)); }

  useit inline bool is_type_sized() const final { return true; }
  useit inline bool is_trivially_copyable() const final { return true; }
  useit inline bool is_trivially_movable() const final { return true; }
  useit inline bool can_be_prerun() const final { return true; }
  useit inline bool can_be_prerun_generic() const final { return true; }
  useit inline bool has_prerun_default_value() const final { return true; }

  useit ir::PrerunValue* get_prerun_default_value(ir::Ctx* irCtx);
  useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;
  useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;
};

} // namespace qat::ir

#endif