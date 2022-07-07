#ifndef QAT_AST_EXPRESSIONS_CUSTOM_INTEGER_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_CUSTOM_INTEGER_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../../utils/types.hpp"
#include "../expression.hpp"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace AST {

class CustomIntegerLiteral : public Expression {
private:
  std::string value;

  u64 bitWidth;

  bool isUnsigned;

public:
  CustomIntegerLiteral(std::string _value, bool _isUnsigned,
                       unsigned int _bitWidth,
                       utils::FilePlacement _filePlacement);

  llvm::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return qat::AST::NodeType::customIntegerLiteral; }
};
} // namespace AST
} // namespace qat

#endif