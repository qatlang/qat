#ifndef QAT_IR_TYPES_FUNCTION_TYPE_HPP
#define QAT_IR_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"
#include "./type_kind.hpp"
#include <vector>

namespace qat {
namespace IR {

class ArgumentType {
private:
  QatType *type;
  bool variability;

public:
  ArgumentType(QatType *_type, bool _variability);

  QatType *getType();
  bool isVariable();
};

class FunctionType : public QatType {
  QatType *returnType;
  bool isReturnVariable;

  std::vector<ArgumentType *> argTypes;

public:
  FunctionType(QatType *_retType, bool _isReturnValueVariable,
               std::vector<ArgumentType *> _argTypes);

  QatType *getReturnType();

  bool isReturnTypeVariable() const;

  ArgumentType *getArgumentTypeAt(unsigned int index);

  std::vector<ArgumentType *> getArgumentTypes() const;

  unsigned getArgumentCount() const;

  TypeKind typeKind() const { return TypeKind::function; }
};

} // namespace IR
} // namespace qat

#endif