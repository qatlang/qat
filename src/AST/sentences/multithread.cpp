#include "./multithread.hpp"

#define MULTITHREAD_INDEX "multithread'index"
#define MULTITHREAD_RESULT "multithread:result"

namespace qat {
namespace AST {

Multithread::Multithread(Expression *_count, Block *_main, Block *_after,
                         utils::FilePlacement _filePlacement)
    : count(_count), Sentence(_filePlacement), name(std::nullopt),
      cache_block(new Block({}, count->file_placement)),
      call_block(new Block({}, count->file_placement)),
      join_block(new Block({}, count->file_placement)), block(_main),
      after(_after), type(std::nullopt) {}

Multithread::Multithread(Expression *_count, std::string _name, QatType *_type,
                         Block *_main, Block *_after,
                         utils::FilePlacement _filePlacement)
    : count(_count), name(_name), type(_type),
      cache_block(new Block({}, count->file_placement)),
      call_block(new Block({}, count->file_placement)),
      join_block(new Block({}, count->file_placement)), block(_main),
      after(_after), Sentence(_filePlacement) {}

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
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat