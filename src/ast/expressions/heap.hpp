#ifndef QAT_AST_EXPRESSIONS_HEAP_HPP
#define QAT_AST_EXPRESSIONS_HEAP_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class HeapGet : public Expression {
private:
  QatType    *type;
  Expression *count;

public:
  HeapGet(QatType *_type, Expression *_count, utils::FileRange _fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType   nodeType() const final { return NodeType::heapGet; }
  useit nuo::Json toJson() const final;
};

class HeapPut : public Expression {
private:
  Expression *ptr;

public:
  HeapPut(Expression *pointer, utils::FileRange fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType   nodeType() const final { return NodeType::heapPut; }
  useit nuo::Json toJson() const final;
};

} // namespace qat::ast

#endif