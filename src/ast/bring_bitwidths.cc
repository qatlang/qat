#include "./bring_bitwidths.hpp"
#include "./types/integer.hpp"
#include "./types/unsigned.hpp"

namespace qat::ast {

void BringBitwidths::create_module(ir::Mod* mod, ir::Ctx* irCtx) const {
  for (auto typ : broughtTypes) {
    if (typ->type_kind() == AstTypeKind::INTEGER) {
      auto intAst = (IntegerType*)typ;
      // FIXME
      if (intAst->bitWidth > 128) {
        irCtx->Error("Arbitrary integer bitwidths above 128 are not allowed at the moment", fileRange);
      }
      if (mod->has_integer_bitwidth(intAst->bitWidth)) {
        irCtx->Error("This signed integer bitwidth is already brought into the current scope", typ->fileRange);
      } else {
        mod->add_integer_bitwidth(intAst->bitWidth);
      }
    } else if (typ->type_kind() == AstTypeKind::UNSIGNED_INTEGER) {
      auto uintAst = (UnsignedType*)typ;
      // FIXME
      if (uintAst->bitWidth > 128) {
        irCtx->Error("Arbitrary unsigned integer bitwidths above 128 are not allowed at the moment", fileRange);
      }
      if (mod->has_unsigned_bitwidth(uintAst->bitWidth)) {
        irCtx->Error("This unsigned integer bitwidth is already brought into the current scope", typ->fileRange);
      } else {
        mod->add_unsigned_bitwidth(uintAst->bitWidth);
      }
    } else {
      irCtx->Error("Expected either a signed or unsigned integer type to be brought into the current scope",
                   typ->fileRange);
    }
    SHOW("Brought integer bitwidth")
  }
}

Json BringBitwidths::to_json() const {
  Vec<JsonValue> typesJson;
  for (auto typ : broughtTypes) {
    typesJson.push_back(typ->to_json());
  }
  return Json()._("nodeType", "bringBitwidths")._("types", typesJson)._("fileRange", fileRange);
}

} // namespace qat::ast