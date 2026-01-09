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
	HeapGet(Type* _type, Expression* _count, FileRangePtr _fileRange)
	    : Expression(std::move(_fileRange)), type(_type), count(_count) {}

	static HeapGet* create(Type* _type, Expression* _count, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(HeapGet), _type, _count, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(type);
		if (count) {
			UPDATE_DEPS(count);
		}
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::HEAP_GET; }
};

class HeapPut final : public Expression {
  private:
	Expression* ptr;

  public:
	HeapPut(Expression* pointer, FileRangePtr _fileRange) : Expression(std::move(_fileRange)), ptr(pointer) {}

	static HeapPut* create(Expression* _pointer, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(HeapPut), _pointer, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(ptr);
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::HEAP_PUT; }
};

class HeapGrow final : public Expression {
  private:
	Type* type;

	Expression* ptr;
	Expression* count;

  public:
	HeapGrow(Type* _type, Expression* _ptr, Expression* _count, FileRangePtr _fileRange)
	    : Expression(_fileRange), type(_type), ptr(_ptr), count(_count) {}

	static HeapGrow* create(Type* type, Expression* ptr, Expression* count, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(HeapGrow), type, ptr, count, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(type);
		UPDATE_DEPS(ptr);
		UPDATE_DEPS(count);
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::HEAP_GROW; }
};

} // namespace qat::ast

#endif
