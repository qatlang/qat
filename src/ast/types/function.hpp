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

  QatType* type;

public:
  ArgumentType(QatType* type);

  ArgumentType(String name, QatType* type);

  useit bool     hasName() const;
  useit String   getName() const;
  useit QatType* getType();
  useit Json     toJson() const;
};

class FunctionType : public QatType {
private:
  QatType* returnType;

  Vec<ArgumentType*> argTypes;

public:
  FunctionType(QatType* _retType, Vec<ArgumentType*> _argTypes, FileRange _fileRange);

  ~FunctionType() override;

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif