#ifndef QAT_IR_BLOCK_HPP
#define QAT_IR_BLOCK_HPP

#include "../utils/unique_id.hpp"
#include "./local_value.hpp"
#include "definable.hpp"
#include "emittable.hpp"
#include "uniq.hpp"
#include <vector>

namespace qat::IR {

class Function;

class Block : public Definable, public Emittable, public Uniq {
private:
  Function *fn;

  Block *parent;

  Vec<LocalValue *> locals;

public:
  Block(Function *_fn, Block *_parent);

  // Get the parent function of this block
  Function *getFunction();

  // Whether this block is the child of another Block
  useit bool hasParent() const;

  // Get the parent block of this block, if any
  void              setParent(Block *value);
  void              setActive();
  void              defineLLVM(llvmHelper &help) const final;
  void              defineCPP(cpp::File &file) const final;
  void              emitCPP(cpp::File &file) const final;
  useit Block      *getParent();
  useit Block      *getEntryBlock();
  useit bool        hasLocalValue(String name) const;
  useit bool        isSame(Block *other) const;
  useit LocalValue *getLocalValue(String name);
  useit llvm::Value *emitLLVM(llvmHelper &help) const final;
  useit nuo::Json toJson() const;
};

} // namespace qat::IR

#endif
