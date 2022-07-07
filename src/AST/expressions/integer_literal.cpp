#include "./integer_literal.hpp"

namespace qat {
namespace AST {

IntegerLiteral::IntegerLiteral(std::string _value,
                               utils::FilePlacement _filePlacement)
    : value(_value), Expression(_filePlacement) {}

llvm::Value *IntegerLiteral::emit(IR::Context *ctx) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    ctx->throw_error("This expression is not assignable", file_placement);
  }
  return llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->llvmContext), value,
                                10u);
}

void IntegerLiteral::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += (value + " ");
  }
}

backend::JSON IntegerLiteral::toJSON() const {
  return backend::JSON()
      ._("nodeType", "integerLiteral")
      ._("value", value)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat