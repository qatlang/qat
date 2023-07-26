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
  bool isReturnArg;

public:
  ArgumentType(QatType* _type, bool _variability);
  ArgumentType(String _name, QatType* _type, bool _variability);
  ArgumentType(String _name, QatType* type, bool isMemberArg, bool _variability, bool isRetArg);

  useit bool     hasName() const;
  useit String   getName() const;
  useit QatType* getType();
  useit bool     isVariable() const;
  useit bool     isMemberArgument() const;
  useit bool     isReturnArgument() const;
  useit String   toString() const;
};

class FunctionType : public QatType {
  QatType*           returnType;
  Vec<ArgumentType*> argTypes;

public:
  FunctionType(QatType* _retType, Vec<ArgumentType*> _argTypes, llvm::LLVMContext& ctx);

  useit QatType*      getReturnType();
  useit ArgumentType* getArgumentTypeAt(u32 index);
  useit Vec<ArgumentType*> getArgumentTypes() const;
  useit u64                getArgumentCount() const;
  useit TypeKind           typeKind() const final { return TypeKind::function; }
  useit String             toString() const final;
  useit bool               hasReturnArgument() const;
  useit IR::QatType* getReturnArgType() const;
};

} // namespace qat::IR

#endif