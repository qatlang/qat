#ifndef QAT_AST_EXPRESSIONS_HEAP_HPP
#define QAT_AST_EXPRESSIONS_HEAP_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class HeapGet final : public Expression {
private:
  Type*       type;
  Expression* count = nullptr;

public:
  HeapGet(Type* _type, Expression* _count, FileRange _fileRange)
      : Expression(std::move(_fileRange)), type(_type), count(_count) {}

  useit static HeapGet* create(Type* _type, Expression* _count, FileRange _fileRange) {
    return std::construct_at(OwnNormal(HeapGet), _type, _count, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(type);
    if (count) {
      UPDATE_DEPS(count);
    }
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::HEAP_GET; }
  useit Json       to_json() const final;
};

class HeapPut final : public Expression {
private:
  Expression* ptr;

public:
  HeapPut(Expression* pointer, FileRange _fileRange) : Expression(std::move(_fileRange)), ptr(pointer) {}

  useit static HeapPut* create(Expression* _pointer, FileRange _fileRange) {
    return std::construct_at(OwnNormal(HeapPut), _pointer, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(ptr);
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::HEAP_PUT; }
  useit Json       to_json() const final;
};

class HeapGrow final : public Expression {
private:
  Type* type;

  Expression* ptr;
  Expression* count;

public:
  HeapGrow(Type* _type, Expression* _ptr, Expression* _count, FileRange _fileRange)
      : Expression(_fileRange), type(_type), ptr(_ptr), count(_count) {}

  useit static HeapGrow* create(Type* type, Expression* ptr, Expression* count, FileRange fileRange) {
    return std::construct_at(OwnNormal(HeapGrow), type, ptr, count, fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(type);
    UPDATE_DEPS(ptr);
    UPDATE_DEPS(count);
  }

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::HEAP_GROW; }
  useit Json       to_json() const final;
};

} // namespace qat::ast

#endif
