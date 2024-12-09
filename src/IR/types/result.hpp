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
	bool	  isPacked = false;

  public:
	ResultType(ir::Type* validType, ir::Type* errorType, bool isPacked, ir::Ctx* irCtx);
	static ResultType* get(ir::Type* validType, ir::Type* errorType, bool isPacked, ir::Ctx* irCtx);

	useit ir::Type* getValidType() const;
	useit ir::Type* getErrorType() const;
	useit bool		is_identical_to_bool() const { return validType->is_void() && errorType->is_void(); }
	useit bool		isTypePacked() const;
	useit bool		is_type_sized() const final { return true; }
	useit bool		is_trivially_copyable() const final {
		 return validType->is_trivially_copyable() && errorType->is_trivially_copyable();
	}
	useit bool is_trivially_movable() const final {
		return validType->is_trivially_movable() && errorType->is_trivially_movable();
	}
	void		   handle_tag_store(llvm::Value* resValue, bool tag, ir::Ctx* irCtx);
	useit TypeKind type_kind() const final { return TypeKind::result; }
	useit String   to_string() const final;
};

} // namespace qat::ir

#endif
