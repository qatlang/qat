#ifndef QAT_IR_TYPES_FUNCTION_TYPE_HPP
#define QAT_IR_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"
#include "./type_kind.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat::IR {

class ArgumentType {
private:
  // Name of the argument
  Maybe<String> name;
  // Type of the argument
  QatType *type;
  // Variability of the argument
  bool variability;

public:
  ArgumentType(QatType *_type, bool _variability);
  ArgumentType(String _name, QatType *_type, bool _variability);

  useit bool     hasName() const;
  useit String   getName() const;
  useit QatType *getType();
  useit bool     isVariable() const;
  useit String   toString() const;
};

class FunctionType : public QatType {
  QatType            *returnType;
  bool                isReturnVariable;
  Vec<ArgumentType *> argTypes;

public:
  FunctionType(QatType *_retType, bool _isReturnValueVariable,
               Vec<ArgumentType *> _argTypes);

  useit QatType      *getReturnType();
  useit bool          isReturnTypeVariable() const;
  useit ArgumentType *getArgumentTypeAt(u32 index);
  useit Vec<ArgumentType *> getArgumentTypes() const;
  useit u64                 getArgumentCount() const;
  useit TypeKind typeKind() const override { return TypeKind::function; }
  useit String   toString() const override;
  useit llvm::Type *emitLLVM(llvmHelper &help) const override;
  useit nuo::Json toJson() const override;
  void            emitCPP(cpp::File &file) const override;
};

} // namespace qat::IR

#endif