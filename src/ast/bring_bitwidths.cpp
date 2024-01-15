#include "./bring_bitwidths.hpp"
#include "./types/integer.hpp"
#include "./types/unsigned.hpp"

namespace qat::ast {

BringBitwidths::BringBitwidths(Vec<ast::QatType*> _broughtTypes, FileRange _fileRange)
    : Node(_fileRange), broughtTypes(_broughtTypes) {}

void BringBitwidths::handleBrings(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  for (auto typ : broughtTypes) {
    if (typ->typeKind() == TypeKind::integer) {
      auto intAst = (IntegerType*)typ;
      // FIXME
      if (intAst->bitWidth > 128) {
        ctx->Error("Arbitrary integer bitwidths above 128 are not allowed at the moment", fileRange);
      }
      if (mod->hasIntegerBitwidth(intAst->bitWidth)) {
        ctx->Error("This signed integer bitwidth is already brought into the current scope", typ->fileRange);
      } else {
        mod->addIntegerBitwidth(intAst->bitWidth);
      }
    } else if (typ->typeKind() == TypeKind::unsignedInteger) {
      auto uintAst = (UnsignedType*)typ;
      // FIXME
      if (uintAst->bitWidth > 128) {
        ctx->Error("Arbitrary unsigned integer bitwidths above 128 are not allowed at the moment", fileRange);
      }
      if (mod->hasUnsignedBitwidth(uintAst->bitWidth)) {
        ctx->Error("This unsigned integer bitwidth is already brought into the current scope", typ->fileRange);
      } else {
        mod->addUnsignedBitwidth(uintAst->bitWidth);
      }
    } else {
      ctx->Error("Expected either a signed or unsigned integer type to be brought into the current scope",
                 typ->fileRange);
    }
    SHOW("Brought integer bitwidth")
  }
}

Json BringBitwidths::toJson() const {
  Vec<JsonValue> typesJson;
  for (auto typ : broughtTypes) {
    typesJson.push_back(typ->toJson());
  }
  return Json()._("nodeType", "bringBitwidths")._("types", typesJson)._("fileRange", fileRange);
}

} // namespace qat::ast