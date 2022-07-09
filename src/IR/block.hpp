#ifndef QAT_IR_BLOCK_HPP
#define QAT_IR_BLOCK_HPP

#include "../utils/unique_id.hpp"
#include "./local_value.hpp"
#include <vector>

namespace qat {
namespace IR {

class Function;

class Block {
private:
  std::string id;

  Function *fn;

  Block *parent;

  std::vector<LocalValue *> locals;

public:
  Block(Function *_fn, Block *_parent);

  std::string getID() const;

  Function *getFunction();

  bool hasParent() const;

  Block *getParent();

  Block *getEntryBlock();

  void setParent(Block *value);

  void setActive();

  bool hasLocalValue(std::string name) const;

  bool isSame(Block *other) const;

  LocalValue *getLocalValue(std::string name);
};

} // namespace IR
} // namespace qat

#endif