#ifndef QAT_IR_LOCAL_VALUE_HPP
#define QAT_IR_LOCAL_VALUE_HPP

#include "./value.hpp"
#include <string>

namespace qat::ast {
class QatType;
}

namespace qat::IR {

class Block;
class Context;

class LocalValue : public Value {
private:
  // Name of the value
  String name;

  Value *initial;

  Block *parent;

  u64 loads;

  u64 stores;

  u64 refers;

public:
  LocalValue(String _name, IR::QatType *_type, bool _isVariable,
             Value *_initial, Block *block);

  String getName() const;

  Block *getParent();

  u64 getLoads() const;

  u64 getStores() const;

  u64 getRefers() const;

  bool isRemovable() const;

  nuo::Json toJson() const;
};

} // namespace qat::IR

#endif