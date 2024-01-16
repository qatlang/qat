#ifndef QAT_AST_TYPES_FUNCTION_TYPE_HPP
#define QAT_AST_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"

namespace qat::ast {

class ArgumentType {
private:
  Maybe<String> name;
  QatType*      type;

public:
  explicit ArgumentType(QatType* _type) : type(_type) {}
  ArgumentType(String _name, QatType* _type) : name(_name), type(_type) {}

  useit static inline ArgumentType* create(QatType* type) { return std::construct_at(OwnNormal(ArgumentType), type); }
  useit static inline ArgumentType* create(String _name, QatType* _type) {
    return std::construct_at(OwnNormal(ArgumentType), _name, _type);
  }

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
  FunctionType(QatType* _retType, Vec<ArgumentType*> _argTypes, FileRange _fileRange)
      : QatType(_fileRange), returnType(_retType), argTypes(_argTypes) {}

  useit static inline FunctionType* create(QatType* _retType, Vec<ArgumentType*> _argTypes, FileRange _fileRange) {
    return std::construct_at(OwnNormal(FunctionType), _retType, _argTypes, _fileRange);
  }

  ~FunctionType() final;

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit AstTypeKind  typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif