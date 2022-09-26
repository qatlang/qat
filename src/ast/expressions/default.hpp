#ifndef QAT_AST_EXPRESSIONS_DEFAULT_HPP
#define QAT_AST_EXPRESSIONS_DEFAULT_HPP

#include "../expression.hpp"

namespace qat::ast {

class Default : public Expression {
  friend class LocalDeclaration;

private:
  IR::QatType *candidateType = nullptr;

  Maybe<String> irName;
  bool          isVar = false;

public:
  explicit Default(utils::FileRange _fileRange);

  void setType(IR::QatType *typ);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType   nodeType() const final { return NodeType::Default; }
  useit nuo::Json toJson() const final;
};

} // namespace qat::ast

#endif