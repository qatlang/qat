#ifndef QAT_IR_META_INFO_HPP
#define QAT_IR_META_INFO_HPP

#include "../utils/identifier.hpp"
#include "./types/string_slice.hpp"
#include "value.hpp"

namespace qat::IR {

struct MetaInfo {
  MetaInfo(Vec<Pair<Identifier, IR::PrerunValue*>> keyValues, FileRange _fileRange) : fileRange(_fileRange) {
    for (auto& kv : keyValues) {
      keys.push_back(kv.first);
      values.push_back(kv.second);
    }
  }

  Vec<Identifier>       keys;
  Vec<IR::PrerunValue*> values;
  FileRange             fileRange;

  useit bool hasKey(String const& name) const {
    for (auto& k : keys) {
      if (k.value == name) {
        return true;
      }
    }
    return false;
  }
  useit IR::PrerunValue* getValueFor(String const& name) const {
    usize ind = 0;
    for (auto& k : keys) {
      if (k.value == name) {
        return values.at(ind);
      }
      ind++;
    }
    return nullptr;
  }
  useit Maybe<String> getForeignID() const {
    if (hasKey("foreign")) {
      auto* val = getValueFor("foreign");
      return val->getType()->asStringSlice()->toPrerunGenericString(val);
    } else {
      return None;
    }
  }
};

} // namespace qat::IR

#endif