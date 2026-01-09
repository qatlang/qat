#ifndef QAT_AST_META_REGISTERS_HPP
#define QAT_AST_META_REGISTERS_HPP

#include "../expression.hpp"

namespace qat::ast {

class MetaRegisterRead : public Expression {
	bool              isVolatile;
	PrerunExpression* registerName;
	PrerunExpression* registerType;

  public:
	MetaRegisterRead(bool _isVolatile, PrerunExpression* _regName, PrerunExpression* _regType, FileRangePtr _fileRange)
	    : Expression(_fileRange), isVolatile(_isVolatile), registerName(_regName), registerType(_regType) {}

	useit static MetaRegisterRead* create(bool isVolatile, PrerunExpression* registerName,
	                                      PrerunExpression* registerType, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(MetaRegisterRead), isVolatile, registerName, registerType, fileRange);
	}

	void update_dependencies(ir::EmitPhase, Maybe<ir::DependType>, ir::EntityState*, EmitCtx*) final {}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::META_REGISTER_READ; }
};

class MetaRegisterWrite : public Expression {
	PrerunExpression* registerName;
	PrerunExpression* registerType;
	Expression*       value;

  public:
	MetaRegisterWrite(PrerunExpression* _name, PrerunExpression* _type, Expression* _value, FileRangePtr _fileRange)
	    : Expression(_fileRange), registerName(_name), registerType(_type), value(_value) {}

	useit static MetaRegisterWrite* create(PrerunExpression* registerName, PrerunExpression* registerType,
	                                       Expression* value, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(MetaRegisterWrite), registerName, registerType, value, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* state,
	                         EmitCtx* ctx) final {
		value->update_dependencies(phase, dep, state, ctx);
	}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::META_REGISTER_WRITE; }
};

} // namespace qat::ast

#endif
