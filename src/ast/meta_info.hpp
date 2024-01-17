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
    std::set<String>                        keys;
    Vec<Pair<Identifier, IR::PrerunValue*>> resultVec;
    Vec<FileRange>                          valuesRange;
    for (auto& kv : keyValues) {
      if (keys.contains(kv.first.value)) {
        ctx->Error("The key " + ctx->highlightError(kv.first.value) + " is repeating here", kv.first.range);
      }
      auto irVal = kv.second->emit(ctx);
      if (kv.first.value == "foreign" || kv.first.value == "linkName") {
        if (!irVal->getType()->isStringSlice()) {
          ctx->Error("The " + ctx->highlightError(kv.first.value) + " field is expected to be of type " +
                         ctx->highlightError("str") + ". Got an expression of type " +
                         ctx->highlightError(irVal->getType()->toString()),
                     kv.second->fileRange);
        }
      }
      resultVec.push_back({kv.first, irVal});
      valuesRange.push_back(kv.second->fileRange);
    }
    return IR::MetaInfo(resultVec, valuesRange, fileRange);
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