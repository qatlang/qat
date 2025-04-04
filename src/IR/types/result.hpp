#ifndef QAT_IR_TYPES_RESULT_HPP
#define QAT_IR_TYPES_RESULT_HPP

#include "qat_type.hpp"

namespace llvm {
class Value;
}

namespace qat::ir {

class ResultType : public Type {
	friend class Type;

	ir::Type* validType;
	ir::Type* errorType;
	bool      isPacked = false;

  public:
	ResultType(ir::Type* validType, ir::Type* errorType, bool isPacked, ir::Ctx* irCtx);
	static ResultType* get(ir::Type* validType, ir::Type* errorType, bool isPacked, ir::Ctx* irCtx);

	useit ir::Type* get_valid_type() const;

	useit ir::Type* get_error_type() const;

	useit bool is_identical_to_bool() const { return validType->is_void() && errorType->is_void(); }

	useit bool is_packed() const;

	useit bool is_type_sized() const final { return true; }

	useit bool has_simple_copy() const final {
		return is_identical_to_bool() || (validType->has_simple_copy() && errorType->has_simple_copy());
	}

	useit bool has_simple_move() const final {
		return is_identical_to_bool() || (validType->has_simple_move() && errorType->has_simple_move());
	}

	void handle_tag_store(llvm::Value* resValue, bool tag, ir::Ctx* irCtx);

	useit TypeKind type_kind() const final { return TypeKind::RESULT; }

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
