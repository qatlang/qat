#ifndef QAT_AST_EXPRESSIONS_HEAP_HPP
#define QAT_AST_EXPRESSIONS_HEAP_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class HeapGet : public Expression {
private:
  QatType*    type;
  Expression* count = nullptr;

public:
  HeapGet(QatType* _type, Expression* _count, FileRange _fileRange)
      : Expression(std::move(_fileRange)), type(_type), count(_count) {}

  useit static inline HeapGet* create(QatType* _type, Expression* _count, FileRange _fileRange) {
    return std::construct_at(OwnNormal(HeapGet), _type, _count, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::HEAP_GET; }
  useit Json       toJson() const final;
};

class HeapPut : public Expression {
private:
  Expression* ptr;

public:
  HeapPut(Expression* pointer, FileRange _fileRange) : Expression(std::move(_fileRange)), ptr(pointer) {}

  useit static inline HeapPut* create(Expression* _pointer, FileRange _fileRange) {
    return std::construct_at(OwnNormal(HeapPut), _pointer, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::HEAP_PUT; }
  useit Json       toJson() const final;
};

class HeapGrow : public Expression {
private:
  QatType* type;

  Expression* ptr;
  Expression* count;

public:
  HeapGrow(QatType* _type, Expression* _ptr, Expression* _count, FileRange _fileRange)
      : Expression(_fileRange), type(_type), ptr(_ptr), count(_count) {}

  useit static inline HeapGrow* create(QatType* type, Expression* ptr, Expression* count, FileRange fileRange) {
    return std::construct_at(OwnNormal(HeapGrow), type, ptr, count, fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::HEAP_GROW; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif