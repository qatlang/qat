#ifndef QAT_AST_SENTENCES_USE_DECLARATION_HPP
#define QAT_AST_SENTENCES_USE_DECLARATION_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class UseDeclaration final : public Sentence {
	Identifier  name;
	Type*       type;
	Expression* value;

  public:
	UseDeclaration(Identifier _name, Type* _type, Expression* _value, FileRange _fileRange)
	    : Sentence(std::move(_fileRange)), name(std::move(_name)), type(_type), value(_value) {}

	useit static UseDeclaration* create(Identifier name, Type* type, Expression* value, FileRange fileRange) {
		return std::construct_at(OwnNormal(UseDeclaration), std::move(name), type, value, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::USE_DECLARATION; }

	useit Json to_json() const final;
};

} // namespace qat::ast

#endif
