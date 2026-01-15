#ifndef QAT_AST_SENTENCES_LOCAL_DECLARATION_HPP
#define QAT_AST_SENTENCES_LOCAL_DECLARATION_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include "../types/qat_type.hpp"
#include <optional>

namespace qat::ast {

class LocalDeclaration final : public Sentence {
  private:
	bool               variability;
	Identifier         name;
	Type*              type;
	Maybe<Expression*> value;
	bool               isUninitialised;

  public:
	LocalDeclaration(bool isVar, Identifier _name, Type* _type, Maybe<Expression*> _value, bool _isUninitialised,
	                 FileRangePtr _fileRange)
	    : Sentence(_fileRange), variability(isVar), name(_name), type(_type), value(_value),
	      isUninitialised(_isUninitialised) {}

	static LocalDeclaration* create(bool isVar, Identifier name, Type* type, Maybe<Expression*> value,
	                                bool isUninitialised, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(LocalDeclaration), isVar, name, type, value, isUninitialised, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		if (type) {
			UPDATE_DEPS(type);
		}
		if (value.has_value()) {
			UPDATE_DEPS(value.value());
		}
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::LOCAL_DECLARATION; }
};

} // namespace qat::ast

#endif
