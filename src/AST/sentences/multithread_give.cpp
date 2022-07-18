#include "multithread_give.hpp"

namespace qat::AST {

MultithreadGive::MultithreadGive(Expression *_expression,
                                 utils::FilePlacement _filePlacement)
    : Sentence(_filePlacement), expression(_expression) {}

IR::Value *MultithreadGive::emit(IR::Context *ctx) {
  // FIXME - Implement this
}

nuo::Json MultithreadGive::toJson() const {
  return nuo::Json()
      ._("nodeType", "multithreadGive")
      ._("value", expression->toJson())
      ._("filePlacement", file_placement);
}

} // namespace qat::AST