#include "./integer.hpp"
#include "../../IR/types/integer.hpp"
#include <string>

namespace qat::ast {

Maybe<usize> IntegerType::getTypeSizeInBits(EmitCtx* ctx) const {
  return (usize)(ctx->mod->get_llvm_module()->getDataLayout().getTypeAllocSizeInBits(
      llvm::Type::getIntNTy(ctx->irCtx->llctx, bitWidth)));
}

ir::Type* IntegerType::emit(EmitCtx* ctx) {
  if (bitWidth > 128) {
    ctx->Error("Arbitrary integer bitwidths above 128 are not allowed at the moment", fileRange);
  }
  if (isPartOfGeneric || ctx->mod->has_integer_bitwidth(bitWidth)) {
    return ir::IntegerType::get(bitWidth, ctx->irCtx);
  } else {
    ctx->Error("The signed integer bitwidth " + ctx->color(std::to_string(bitWidth)) +
                   " is not allowed to be used since it is not brought into into the module " +
                   ctx->color(ctx->mod->get_name()) + " in file " + ctx->mod->get_file_path(),
               fileRange);
  }
  return nullptr;
}

bool IntegerType::isBitWidth(const u32 width) const { return bitWidth == width; }

AstTypeKind IntegerType::type_kind() const { return AstTypeKind::INTEGER; }

Json IntegerType::to_json() const {
  return Json()._("type_kind", "integer")._("bitWidth", bitWidth)._("fileRange", fileRange);
}

String IntegerType::to_string() const { return "i" + std::to_string(bitWidth); }

} // namespace qat::ast