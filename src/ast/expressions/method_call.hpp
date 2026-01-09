#ifndef QAT_AST_EXPRESSIONS_MEMBER_FUNCTION_CALL_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_FUNCTION_CALL_HPP

#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

enum class MethodCallNature : u8 {
	NONE,
	VAR,
};

enum class MethodDisambiguity : u8 {
	NONE,
	TYPE,
	SKILL,
	DONE_SKILL,
};

class MethodCall final : public Expression {
  private:
	Expression*        instance;
	bool               isExpSelf = false;
	Identifier         memberName;
	Vec<Expression*>   arguments;
	MethodCallNature   callNature;
	MethodDisambiguity disambiguity;
	Maybe<Identifier>  doneSkill;

  public:
	MethodCall(Expression* _instance, bool _isExpSelf, Identifier _memberName, Vec<Expression*> _arguments,
	           MethodCallNature _callNature, MethodDisambiguity _disambiguity, Maybe<Identifier> _doneSkill,
	           FileRangePtr _fileRange)
	    : Expression(_fileRange), instance(_instance), isExpSelf(_isExpSelf), memberName(std::move(_memberName)),
	      arguments(std::move(_arguments)), callNature(_callNature), disambiguity(_disambiguity),
	      doneSkill(std::move(_doneSkill)) {}

	static MethodCall* create(Expression* instance, bool isExpSelf, Identifier memberName, Vec<Expression*> arguments,
	                          MethodCallNature callNature, MethodDisambiguity disambiguity, Maybe<Identifier> doneSkill,
	                          FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(MethodCall), instance, isExpSelf, std::move(memberName),
		                         std::move(arguments), callNature, disambiguity, std::move(doneSkill), fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	ir::Value* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::MEMBER_FUNCTION_CALL; }
};

enum class MethodHolderKind : u8 {
	EXPANDED,
	DEFAULT_IMPL,
	UNNAMED_IMPL,
	NAMED_IMPL,
};

enum class MethodQueryType : u8 {
	NORMAL,
	VARIATION,
	VALUED,
};

struct MethodQuery {
	MethodQueryType  methodType;
	MethodHolderKind kind;
	void*            holder;

	String kind_to_string(ir::Ctx* irCtx) const;

	String to_disambiguity(ir::Ctx* irCtx, bool isSelfCall, String const& name) const;

	String to_location(ir::Ctx* irCtx, String const& name) const;
};

} // namespace qat::ast

#endif
