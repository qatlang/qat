#ifndef QAT_IR_META_INFO_HPP
#define QAT_IR_META_INFO_HPP

#include "../utils/identifier.hpp"
#include "./types/string_slice.hpp"
#include "value.hpp"

namespace qat::IR {

struct MetaInfo {
  MetaInfo(Vec<Pair<Identifier, IR::PrerunValue*>> keyValues, Vec<FileRange> _valueRanges, FileRange _fileRange)
      : fileRange(_fileRange), valueRanges(_valueRanges) {
    for (auto& kv : keyValues) {
      keys.push_back(kv.first);
      values.push_back(kv.second);
    }
  }

  Vec<Identifier>       keys;
  Vec<IR::PrerunValue*> values;
  Vec<FileRange>        valueRanges;
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

  useit FileRange getValueRangeFor(String const& name) const {
    usize ind = 0;
    for (auto& k : keys) {
      if (k.value == name) {
        return valueRanges.at(ind);
      }
      ind++;
    }
  }

  useit Maybe<String> getForeignID() const {
    if (hasKey("foreign")) {
      return IR::StringSliceType::value_to_string(getValueFor("foreign"));
    } else {
      return None;
    }
  }

  useit Maybe<String> getValueAsStringFor(String key) const {
    if (hasKey(key)) {
      return IR::StringSliceType::value_to_string(getValueFor(key));
    } else {
      return None;
    }
  }
};

} // namespace qat::IR

#endif