#ifndef QAT_IR_TYPES_FUNCTION_TYPE_HPP
#define QAT_IR_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"
#include "./type_kind.hpp"
#include "llvm/IR/LLVMContext.h"
#include <optional>
#include <string>
#include <vector>

namespace qat::ir {

class ArgumentType {
private:
  // Name of the argument
  Maybe<String> name;
  // Type of the argument
  Type* type;
  // Variability of the argument
  bool variability;
  bool isMemberArg;

public:
  ArgumentType(Type* _type, bool _variability);
  ArgumentType(String _name, Type* _type, bool _variability);
  ArgumentType(String _name, Type* type, bool is_member_argument, bool _variability);

  useit bool   has_name() const;
  useit String get_name() const;
  useit Type*  get_type();
  useit bool   is_variable() const;
  useit bool   is_member_argument() const;
  useit String to_string() const;
};

class ReturnType {
private:
  Type* retTy;
  bool  isReturnSelfRef;

  ReturnType(Type* _retTy, bool _isReturnSelfRef);

public:
  useit static ReturnType* get(Type* _retTy);
  useit static ReturnType* get(Type* _retTy, bool _isRetSelf);

  useit Type*  get_type() const;
  useit bool   is_return_self() const;
  useit String to_string() const;
};

class FunctionType final : public Type {
  ReturnType*        returnType;
  Vec<ArgumentType*> argTypes;

public:
  FunctionType(ReturnType* _retType, Vec<ArgumentType*> _argTypes, llvm::LLVMContext& ctx);

  ~FunctionType() final;

  useit ReturnType*   get_return_type();
  useit ArgumentType* get_argument_type_at(u32 index);
  useit Vec<ArgumentType*> get_argument_types() const;
  useit u64                get_argument_count() const;
  useit TypeKind           type_kind() const final { return TypeKind::function; }
  useit String             to_string() const final;
};

} // namespace qat::ir

#endif