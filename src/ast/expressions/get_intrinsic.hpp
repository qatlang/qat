#ifndef QAT_AST_EXPRESSION_GET_INTRINSIC_HPP
#define QAT_AST_EXPRESSION_GET_INTRINSIC_HPP

#include "../expression.hpp"

namespace qat::ast {

// NOTE - !!! Update the standard library if this is updated
enum class IntrinsicID : u32 {
  llvm_matrix_multiply,
  llvm_matrix_transpose,
};

class GetIntrinsic : public Expression {
  Vec<PrerunExpression*> args;

public:
  GetIntrinsic(Vec<PrerunExpression*> _types, FileRange _fileRange) : Expression(_fileRange), args(_types) {}

  useit static inline GetIntrinsic* create(Vec<PrerunExpression*> _args, FileRange _fileRange) {
    return std::construct_at(OwnNormal(GetIntrinsic), _args, _fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::GET_INTRINSIC; }
};

} // namespace qat::ast

#endif