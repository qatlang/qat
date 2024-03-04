#ifndef QAT_AST_META_INFO_HPP
#define QAT_AST_META_INFO_HPP

#include "expression.hpp"

namespace qat::ast {

struct MetaInfo {
  Vec<Pair<Identifier, PrerunExpression*>> keyValues;
  FileRange                                fileRange;

  MetaInfo(Vec<Pair<Identifier, PrerunExpression*>> _keyValues, FileRange _fileRange)
      : keyValues(_keyValues), fileRange(_fileRange) {}

  useit ir::MetaInfo toIR(EmitCtx* ctx) const {
    std::set<String>                        keys;
    Vec<Pair<Identifier, ir::PrerunValue*>> resultVec;
    Vec<FileRange>                          valuesRange;
    for (auto& kv : keyValues) {
      if (keys.contains(kv.first.value)) {
        ctx->Error("The key " + ctx->color(kv.first.value) + " is repeating here", kv.first.range);
      }
      auto irVal = kv.second->emit(ctx);
      if (kv.first.value == "foreign" || kv.first.value == ir::MetaInfo::linkAsKey) {
        if (!irVal->get_ir_type()->is_string_slice()) {
          ctx->Error("The " + ctx->color(kv.first.value) + " field is expected to be of type " + ctx->color("str") +
                         ". Got an expression of type " + ctx->color(irVal->get_ir_type()->to_string()),
                     kv.second->fileRange);
        }
      }
      resultVec.push_back({kv.first, irVal});
      valuesRange.push_back(kv.second->fileRange);
    }
    return ir::MetaInfo(resultVec, valuesRange, fileRange);
  }

  useit Json to_json() const {
    Vec<JsonValue> kvJson;
    for (auto& kv : keyValues) {
      kvJson.push_back(Json()._("key", kv.first)._("value", kv.second->to_json()));
    }
    return Json()._("keyValues", kvJson)._("fileRange", fileRange);
  }
};

} // namespace qat::ast

#endif