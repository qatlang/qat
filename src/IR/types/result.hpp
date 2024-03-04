#ifndef QAT_IR_TYPES_RESULT_HPP
#define QAT_IR_TYPES_RESULT_HPP

#include "qat_type.hpp"

namespace qat::ir {

class ResultType : public Type {
  friend class Type;

  ir::Type* validType;
  ir::Type* errorType;
  bool      isPacked = false;

  ResultType(ir::Type* validType, ir::Type* errorType, bool isPacked, ir::Ctx* irCtx);

public:
  static ResultType* get(ir::Type* validType, ir::Type* errorType, bool isPacked, ir::Ctx* irCtx);

  useit ir::Type* getValidType() const;
  useit ir::Type* getErrorType() const;
  useit bool      isTypePacked() const;
  useit bool      is_type_sized() const final;
  useit bool      is_trivially_copyable() const final {
    return validType->is_trivially_copyable() && errorType->is_trivially_copyable();
  }
  useit bool is_trivially_movable() const final {
    return validType->is_trivially_movable() && errorType->is_trivially_movable();
  }
  useit TypeKind type_kind() const final;
  useit String   to_string() const final;
};

} // namespace qat::ir

#endif
