#ifndef QAT_AST_MEMBER_INITIATLISATION_HPP
#define QAT_AST_MEMBER_INITIATLISATION_HPP

#include "../expression.hpp"
#include "../sentence.hpp"

namespace qat::ast {

class MemberInit final : public Sentence {
	friend class ConstructorDefinition;
	friend class ConvertorDefinition;

	Identifier  memName;
	Expression* value;
	bool        isInitOfMixVariantWithoutValue;

	bool isAllowed = false;

  public:
	MemberInit(Identifier _memName, Expression* _value, bool _isInitOfMixVariantWithoutValue, FileRangePtr _fileRange)
	    : Sentence(_fileRange), memName(_memName), value(_value),
	      isInitOfMixVariantWithoutValue(_isInitOfMixVariantWithoutValue) {}

	static MemberInit* create(Identifier _memName, Expression* _value, bool _isInitOfMixVariantWithoutValue,
	                          FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(MemberInit), _memName, _value, _isInitOfMixVariantWithoutValue, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(value);
	}

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::MEMBER_INIT; }
};

} // namespace qat::ast

#endif
