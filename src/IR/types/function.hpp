#ifndef QAT_IR_TYPES_FUNCTION_TYPE_HPP
#define QAT_IR_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"
#include "./type_kind.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat {
namespace IR {

class ArgumentType {
private:
  // Name of the argument
  std::optional<std::string> name;

  // Type of the argument
  QatType *type;

  // Variability of the argument
  bool variability;

public:
  ArgumentType(QatType *_type, bool _variability);
  ArgumentType(std::string _name, QatType *_type, bool _variability);

  bool hasName() const;

  std::string getName() const;

  QatType *getType();

  bool isVariable() const;
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