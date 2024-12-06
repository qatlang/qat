#ifndef QAT_AST_META_INFO_HPP
#define QAT_AST_META_INFO_HPP

#include "expression.hpp"

namespace qat::ast {

struct MetaInfo {
  Vec<Pair<Identifier, PrerunExpression*>> keyValues;
  Maybe<FileRange>                         inlineRange;
  FileRange                                fileRange;

  mutable bool isParentFunction = false;

  MetaInfo(Vec<Pair<Identifier, PrerunExpression*>> _keyValues, FileRange _fileRange)
      : keyValues(_keyValues), fileRange(_fileRange) {}

  void set_parent_as_function() const { isParentFunction = true; }

  useit bool has_inline_range() const { return inlineRange.has_value(); }

  useit ir::MetaInfo toIR(EmitCtx* ctx) const {
    std::set<String>                        keys;
    Vec<Pair<Identifier, ir::PrerunValue*>> resultVec;
    Vec<FileRange>                          valuesRange;
    for (auto& kv : keyValues) {
      if (keys.contains(kv.first.value)) {
        ctx->Error("The key " + ctx->color(kv.first.value) + " is repeating here", kv.first.range);
      }
      auto irVal = kv.second->emit(ctx);
      if (kv.first.value == ir::MetaInfo::foreignKey || kv.first.value == ir::MetaInfo::linkAsKey ||
          kv.first.value == ir::MetaInfo::providesKey) {
        if (not irVal->get_ir_type()->is_string_slice()) {
          ctx->Error("The " + ctx->color(kv.first.value) + " field is expected to be of type " +
                         ctx->color(ir::StringSliceType::get(ctx->irCtx, false)->to_string()) +
                         ". Got an expression of type " + ctx->color(irVal->get_ir_type()->to_string()) + " instead",
                     kv.second->fileRange);
        }
      } else if (kv.first.value == ir::MetaInfo::inlineKey) {
        if (!isParentFunction) {
          ctx->Error("This " + ctx->color("meta") + " info does not belong to a function, and hence the " +
                         ctx->color("inline") + " keyword cannot be used",
                     kv.second->fileRange);
        }
        if (inlineRange.has_value()) {
          ctx->Error(
              "The " + ctx->color(ir::MetaInfo::inlineKey) + " condition is repeating here",
              inlineRange.value().is_before(kv.second->fileRange) ? kv.second->fileRange : inlineRange.value(),
              Pair<String, FileRange>{
                  "The " + ctx->color(ir::MetaInfo::inlineKey) + " condition is already provided here",
                  inlineRange.value().is_before(kv.second->fileRange) ? inlineRange.value() : kv.second->fileRange});
        }
        if (!irVal->get_ir_type()->is_bool()) {
          ctx->Error("The " + ctx->color(ir::MetaInfo::inlineKey) + " field is expected to be of type " +
                         ctx->color("bool") + ". Got an expression of type " +
                         ctx->color(irVal->get_ir_type()->to_string()) + " instead",
                     kv.second->fileRange);
        }
      }
      resultVec.push_back({kv.first, irVal});
      valuesRange.push_back(kv.second->fileRange);
    }
    if (inlineRange.has_value()) {
      if (!isParentFunction) {
        ctx->Error("This " + ctx->color("meta") + " info does not belong to a parent, and hence the " +
                       ctx->color("inline") + " keyword cannot be used here",
                   inlineRange);
      }
      resultVec.push_back({{ir::MetaInfo::inlineKey, inlineRange.value()},
                           ir::PrerunValue::get(llvm::ConstantInt::getBool(ctx->irCtx->llctx, false),
                                                ir::UnsignedType::create_bool(ctx->irCtx))});
      valuesRange.push_back(inlineRange.value());
    }
    return ir::MetaInfo(resultVec, valuesRange, fileRange);
  }

  useit Json to_json() const {
    Vec<JsonValue> kvJson;
    for (auto& kv : keyValues) {
      kvJson.push_back(Json()._("key", kv.first)._("value", kv.second->to_json()));
    }
    return Json()
        ._("keyValues", kvJson)
        ._("hasInline", inlineRange.has_value())
        ._("inlineRange", inlineRange.has_value() ? inlineRange.value() : JsonValue())
        ._("fileRange", fileRange);
  }
};

} // namespace qat::ast

#endif
