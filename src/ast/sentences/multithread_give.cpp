#include "multithread_give.hpp"

namespace qat::ast {

MultithreadGive::MultithreadGive(Expression* _expression, FileRange _fileRange)
    : Sentence(_fileRange), expression(_expression) {}

IR::Value* MultithreadGive::emit(IR::Context* ctx) {
  // FIXME - Implement this
return nullptr;
}

Json MultithreadGive::toJson() const {
  return Json()._("nodeType", "multithreadGive")._("value", expression->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast