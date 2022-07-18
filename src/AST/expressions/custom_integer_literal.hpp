#ifndef QAT_AST_EXPRESSIONS_CUSTOM_INTEGER_LITERAL_HPP
#define QAT_AST_EXPRESSIONS_CUSTOM_INTEGER_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../../utils/types.hpp"
#include "../expression.hpp"

namespace qat::AST {

class CustomIntegerLiteral : public Expression {
private:
  std::string value;

  u64 bitWidth;

  bool isUnsigned;

public:
  CustomIntegerLiteral(std::string _value, bool _isUnsigned,
                       unsigned int _bitWidth, utils::FileRange _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return qat::AST::NodeType::customIntegerLiteral; }
};

} // namespace qat::AST

#endif