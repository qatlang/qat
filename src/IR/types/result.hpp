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

	static Vec<ResultType*> allResultTypes;

  public:
	ResultType(ir::Type* validType, ir::Type* errorType, bool isPacked, ir::Ctx* irCtx);

	static ResultType* get(ir::Type* validType, ir::Type* errorType, bool isPacked, ir::Ctx* irCtx);

	ir::Type* get_valid_type() const;

	ir::Type* get_error_type() const;

	bool is_packed() const { return isPacked; }

	bool is_type_sized() const final { return true; }

	bool has_simple_copy() const final { return (validType->has_simple_copy() && errorType->has_simple_copy()); }

	bool has_simple_move() const final { return (validType->has_simple_move() && errorType->has_simple_move()); }

	void handle_tag_store(llvm::Value* resValue, bool tag, ir::Ctx* irCtx);

	TypeKind type_kind() const final { return TypeKind::RESULT; }

	String to_string() const final;
};

} // namespace qat::ir

#endif
