#ifndef QAT_PRERUN_SENTENCE_HPP
#define QAT_PRERUN_SENTENCE_HPP

#include "../../IR/context.hpp"

namespace qat::ast {

class PrerunSentence {
protected:
  FileRange fileRange;

public:
  PrerunSentence(FileRange _range) : fileRange(_range) {}

  virtual void execute(ir::Ctx* irCtx) = 0;
};

} // namespace qat::ast

#endif