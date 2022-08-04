#include "./multithread.hpp"

#define MULTITHREAD_INDEX  "multithread'index"
#define MULTITHREAD_RESULT "multithread:result"

namespace qat::ast {

Multithread::Multithread(Expression *_count, Block *_main, Block *_after,
                         utils::FileRange _fileRange)
    : count(_count), Sentence(_fileRange), name(None),
      cache_block(new Block({}, count->fileRange)),
      call_block(new Block({}, count->fileRange)),
      join_block(new Block({}, count->fileRange)), block(_main), after(_after),
      type(None) {}

Multithread::Multithread(Expression *_count, String _name, QatType *_type,
                         Block *_main, Block *_after,
                         utils::FileRange _fileRange)
    : count(_count), name(_name), type(_type),
      cache_block(new Block({}, count->fileRange)),
      call_block(new Block({}, count->fileRange)),
      join_block(new Block({}, count->fileRange)), block(_main), after(_after),
      Sentence(_fileRange) {}

IR::Value *Multithread::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json Multithread::toJson() const {
  return nuo::Json()
      ._("nodeType", "multithread")
      ._("count", count->toJson())
      ._("hasCollect", name.has_value())
      ._("collect", nuo::Json()
                        ._("name", name.value_or(""))
                        ._("type", type.has_value() ? type.value()->toJson()
                                                    : nuo::Json()))
      ._("mainBlock", block->toJson())
      ._("afterBlock", after->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast