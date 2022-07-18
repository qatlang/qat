#ifndef QAT_AST_TYPES_FUNCTION_TYPE_HPP
#define QAT_AST_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat {
namespace AST {

class ArgumentType {
private:
  std::optional<std::string> name;

  QatType *type;

public:
  ArgumentType(QatType *type);

  ArgumentType(std::string name, QatType *type);

  bool hasName() const;

  std::string getName() const;

  QatType *getType();

  nuo::Json toJson() const;
};

class FunctionType : public QatType {
private:
  QatType *returnType;

  std::vector<ArgumentType *> argTypes;

public:
  FunctionType(QatType *_retType, std::vector<ArgumentType *> _argTypes,
               utils::FilePlacement _placement);

  IR::QatType *emit(IR::Context *ctx);

  TypeKind typeKind() const;

  nuo::Json toJson() const;

  std::string toString() const;
};

} // namespace AST
} // namespace qat

#endif