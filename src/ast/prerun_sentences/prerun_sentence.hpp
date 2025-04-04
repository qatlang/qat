#ifndef QAT_AST_PRERUN_SENTENCE_HPP
#define QAT_AST_PRERUN_SENTENCE_HPP

#include "../emit_ctx.hpp"

namespace qat::ast {

class PrerunSentence {
  protected:
	FileRange fileRange;

  public:
	PrerunSentence(FileRange _fileRange) : fileRange(_fileRange) {}

	virtual void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                                 EmitCtx* ctx) {}

	virtual void emit(EmitCtx* ctx) = 0;
};

} // namespace qat::ast

#endif
