#include "./give_sentence.hpp"
namespace qat {
namespace AST {

GiveSentence::GiveSentence(std::optional<Expression *> _given_expr,
                           utils::FilePlacement _filePlacement)
    : give_expr(_given_expr), Sentence(_filePlacement) {}

llvm::Value *GiveSentence::generate(IR::Generator *generator) {
  auto block = generator->builder.GetInsertBlock();
  auto parent = block->getParent();
  auto ret_type = parent->getReturnType();
  if (give_expr.has_value()) {
    auto expr = give_expr.value();
    auto val = expr->generate(generator);
    if (val->getType() == ret_type) {
      return llvm::ReturnInst::Create(generator->llvmContext, val, block);
    } else {
      generator->throw_error(
          "Function `" + block->getParent()->getName().str() +
              "` expects type of `" + utils::llvmTypeToName(ret_type) +
              "` to be given back but the provided value is of type `" +
              utils::llvmTypeToName(val->getType()) + "`",
          file_placement);
    }
  } else {
    if (ret_type != llvm::Type::getVoidTy(generator->llvmContext)) {
      generator->throw_error(
          "Function `" + parent->getName().str() + "` expects type of `" +
              utils::llvmTypeToName(ret_type) +
              "` to be given back but the provided value is `void`",
          file_placement);
    }
    return llvm::ReturnInst::Create(generator->llvmContext, block);
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