#ifndef QAT_IR_LOCAL_VALUE_HPP
#define QAT_IR_LOCAL_VALUE_HPP

#include "./value.hpp"
#include <string>

namespace qat {

namespace AST {
class QatType;
}

namespace IR {

class Block;
class Context;

class LocalValue : public Value {
private:
  // Name of the value
  std::string name;

  Value *initial;

  Block* parent;

  unsigned loads;

  unsigned stores;

  unsigned refers;

public:
  LocalValue(std::string _name, IR::QatType *_type, bool _isVariable,
             Value *_initial, Block *block);

  std::string getName() const { return name; }

  Block* getParent();

  unsigned getLoads() const { return loads; }

  unsigned getStores() const { return stores; }

  unsigned getRefers() const { return refers; }

  bool isRemovable() const;
};

} // namespace IR
} // namespace qat

#endif