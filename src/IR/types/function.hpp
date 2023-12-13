#ifndef QAT_IR_TYPES_FUNCTION_TYPE_HPP
#define QAT_IR_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"
#include "./type_kind.hpp"
#include "llvm/IR/LLVMContext.h"
#include <optional>
#include <string>
#include <vector>

namespace qat::IR {

class ArgumentType {
private:
  // Name of the argument
  Maybe<String> name;
  // Type of the argument
  QatType* type;
  // Variability of the argument
  bool variability;
  bool isMemberArg;

public:
  ArgumentType(QatType* _type, bool _variability);
  ArgumentType(String _name, QatType* _type, bool _variability);
  ArgumentType(String _name, QatType* type, bool isMemberArg, bool _variability);

  useit bool     hasName() const;
  useit String   getName() const;
  useit QatType* getType();
  useit bool     isVariable() const;
  useit bool     isMemberArgument() const;
  useit String   toString() const;
};

class ReturnType {
private:
  QatType* retTy;
  bool     isReturnSelfRef;

  ReturnType(QatType* _retTy, bool _isReturnSelfRef);

public:
  useit static ReturnType* get(QatType* _retTy);
  useit static ReturnType* get(QatType* _retTy, bool _isRetSelf);

  useit QatType* getType() const;
  useit bool     isReturnSelf() const;
  useit String   toString() const;
};

class FunctionType final : public QatType {
  ReturnType*        returnType;
  Vec<ArgumentType*> argTypes;

public:
  FunctionType(ReturnType* _retType, Vec<ArgumentType*> _argTypes, llvm::LLVMContext& ctx);

  ~FunctionType() final;

  useit ReturnType*   getReturnType();
  useit ArgumentType* getArgumentTypeAt(u32 index);
  useit Vec<ArgumentType*> getArgumentTypes() const;
  useit u64                getArgumentCount() const;
  useit TypeKind           typeKind() const final { return TypeKind::function; }
  useit String             toString() const final;
};

} // namespace qat::IR

#endif