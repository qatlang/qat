#ifndef QAT_AST_EXPRESSIONS_MEMBER_FUNCTION_CALL_HPP
#define QAT_AST_EXPRESSIONS_MEMBER_FUNCTION_CALL_HPP

#include "../../IR/context.hpp"
#include "../../utils/pointer_kind.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include "llvm/IR/Value.h"
#include <string>
#include <vector>

namespace qat {
namespace AST {
class MemberFunctionCall : public Expression {
private:
  Expression *instance;
  std::string memberName;
  std::vector<Expression *> arguments;
  bool variation;

public:
  MemberFunctionCall(Expression *_instance, std::string _memberName,
                     std::vector<Expression *> _arguments, bool _variation,
                     utils::FilePlacement _filePlacement)
      : instance(_instance), memberName(_memberName), arguments(_arguments),
        variation(_variation), Expression(_filePlacement) {}

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return qat::AST::NodeType::memberFunctionCall; }
};
} // namespace AST
} // namespace qat

#endif