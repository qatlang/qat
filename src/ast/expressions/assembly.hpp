#ifndef QAT_AST_EXPRESSION_INLINE_ASSEMBLY_HPP
#define QAT_AST_EXPRESSION_INLINE_ASSEMBLY_HPP

#include "../expression.hpp"

namespace qat::ast {

class InlineAssembly : public Expression {
  private:
	Type*               functionType;
	PrerunExpression*   asmValue;
	Vec<Expression*>    arguments;
	FileRangePtr        argsRange;
	PrerunExpression*   clobbers;
	PrerunExpression*   volatileExp;
	Maybe<FileRangePtr> volatileRange;

  public:
	InlineAssembly(Type* _functionType, PrerunExpression* _asmValue, Vec<Expression*> _arguments,
	               FileRangePtr _argsRange, PrerunExpression* _clobbers, Maybe<FileRangePtr> _volatileRange,
	               PrerunExpression* _volatileExp, FileRangePtr _fileRange)
	    : Expression(_fileRange), functionType(_functionType), asmValue(_asmValue), arguments(_arguments),
	      argsRange(_argsRange), clobbers(_clobbers), volatileExp(_volatileExp), volatileRange(_volatileRange) {}

	useit static InlineAssembly* create(Type* functionType, PrerunExpression* asmValue, Vec<Expression*> arguments,
	                                    FileRangePtr argsRange, PrerunExpression* clobbers,
	                                    Maybe<FileRangePtr> volatileRange, PrerunExpression* volatileExp,
	                                    FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(InlineAssembly), functionType, asmValue, arguments, argsRange, clobbers,
		                         volatileRange, volatileExp, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::Value* emit(EmitCtx* ctx) final;
	useit Json       to_json() const final;

	useit NodeType nodeType() const final { return NodeType::INLINE_ASSEMBLY; }
};

} // namespace qat::ast

#endif
