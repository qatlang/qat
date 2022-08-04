#ifndef QAT_AST_TYPES_FUNCTION_TYPE_HPP
#define QAT_AST_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat::ast {

class ArgumentType {
private:
  Maybe<String> name;

  QatType *type;

public:
  ArgumentType(QatType *type);

  ArgumentType(String name, QatType *type);

  bool hasName() const;

  String getName() const;

  QatType *getType();

  nuo::Json toJson() const;
};

class FunctionType : public QatType {
private:
  QatType *returnType;

  Vec<ArgumentType *> argTypes;

public:
  FunctionType(QatType *_retType, Vec<ArgumentType *> _argTypes,
               utils::FileRange _fileRange);

  IR::QatType *emit(IR::Context *ctx);

  TypeKind typeKind() const;

  nuo::Json toJson() const;

  String toString() const;
};

} // namespace qat::ast

#endif