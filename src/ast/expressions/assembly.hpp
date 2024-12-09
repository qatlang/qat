#ifndef QAT_AST_EXPRESSION_ASSEMBLY_HPP
#define QAT_AST_EXPRESSION_ASSEMBLY_HPP

#include "../expression.hpp"

namespace qat::ast {

class AssemblyBlock : public Expression {
  private:
	Type*             functionType;
	PrerunExpression* asmValue;
	Vec<Expression*>  arguments;
	FileRange         argsRange;
	PrerunExpression* clobbers;
	PrerunExpression* volatileExp;
	Maybe<FileRange>  volatileRange;

  public:
	AssemblyBlock(Type* _functionType, PrerunExpression* _asmValue, Vec<Expression*> _arguments, FileRange _argsRange,
	              PrerunExpression* _clobbers, Maybe<FileRange> _volatileRange, PrerunExpression* _volatileExp,
	              FileRange _fileRange)
	    : Expression(_fileRange), functionType(_functionType), asmValue(_asmValue), arguments(_arguments),
	      argsRange(_argsRange), clobbers(_clobbers), volatileExp(_volatileExp), volatileRange(_volatileRange) {}

	useit static AssemblyBlock* create(Type* functionType, PrerunExpression* asmValue, Vec<Expression*> arguments,
	                                   FileRange argsRange, PrerunExpression* clobbers, Maybe<FileRange> volatileRange,
	                                   PrerunExpression* volatileExp, FileRange fileRange) {
		return std::construct_at(OwnNormal(AssemblyBlock), functionType, asmValue, arguments, argsRange, clobbers,
		                         volatileRange, volatileExp, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

	useit ir::Value* emit(EmitCtx* ctx) final;
	useit Json       to_json() const final;
	useit NodeType   nodeType() const final { return NodeType::ASSEMBLY_BLOCK; }
};

} // namespace qat::ast

#endif
