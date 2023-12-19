#ifndef QAT_AST_META_INFO_HPP
#define QAT_AST_META_INFO_HPP

#include "expression.hpp"

namespace qat::ast {

struct MetaInfo {
  Vec<Pair<Identifier, PrerunExpression*>> keyValues;
  FileRange                                fileRange;

  MetaInfo(Vec<Pair<Identifier, PrerunExpression*>> _keyValues, FileRange _fileRange)
      : keyValues(_keyValues), fileRange(_fileRange) {}

  useit IR::MetaInfo toIR(IR::Context* ctx) const {
    Vec<Pair<Identifier, IR::PrerunValue*>> resultVec;
    for (auto& kv : keyValues) {
      resultVec.push_back({kv.first, kv.second->emit(ctx)});
    }
    return IR::MetaInfo(resultVec, fileRange);
  }

  useit Json toJson() const {
    Vec<JsonValue> kvJson;
    for (auto& kv : keyValues) {
      kvJson.push_back(Json()._("key", kv.first)._("value", kv.second->toJson()));
    }
    return Json()._("keyValues", kvJson)._("fileRange", fileRange);
  }
};

} // namespace qat::ast

#endif