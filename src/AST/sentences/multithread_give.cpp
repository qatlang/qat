#include "multithread_give.hpp"

namespace qat {
namespace AST {

MultithreadGive::MultithreadGive(Expression _expression,
                                 utils::FilePlacement _filePlacement)
    : expression(_expression), Sentence(_filePlacement) {}

IR::Value *MultithreadGive::emit(IR::Context *ctx) {
  // FIXME - Implement this
}

} // namespace AST
} // namespace qat