#include "./give_sentence.hpp"
namespace qat {
namespace AST {

GiveSentence::GiveSentence(std::optional<Expression *> _given_expr,
                           utils::FilePlacement _filePlacement)
    : give_expr(_given_expr), Sentence(_filePlacement) {}

llvm::Value *GiveSentence::emit(IR::Context *ctx) {
  auto block = ctx->builder.GetInsertBlock();
  auto parent = block->getParent();
  auto ret_type = parent->getReturnType();
  if (give_expr.has_value()) {
    auto expr = give_expr.value();
    auto val = expr->emit(ctx);
    if (val->getType() == ret_type) {
      return llvm::ReturnInst::Create(ctx->llvmContext, val, block);
    } else {
      ctx->throw_error(
          "Function `" + block->getParent()->getName().str() +
              "` expects type of `" + utils::llvmTypeToName(ret_type) +
              "` to be given back but the provided value is of type `" +
              utils::llvmTypeToName(val->getType()) + "`",
          file_placement);
    }
  } else {
    if (ret_type != llvm::Type::getVoidTy(ctx->llvmContext)) {
      ctx->throw_error(
          "Function `" + parent->getName().str() + "` expects type of `" +
              utils::llvmTypeToName(ret_type) +
              "` to be given back but the provided value is `void`",
          file_placement);
    }
    return llvm::ReturnInst::Create(ctx->llvmContext, block);
  }
}

void GiveSentence::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "return ";
    if (give_expr.has_value()) {
      give_expr.value()->emitCPP(file, isHeader);
    }
    file += ";";
  }
}

backend::JSON GiveSentence::toJSON() const {
  return backend::JSON()
      ._("nodeType", "giveSentence")
      ._("hasValue", give_expr.has_value())
      ._("value",
         give_expr.has_value() ? give_expr.value()->toJSON() : backend::JSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat