#ifndef QAT_AST_GLOBAL_DECLARATION_HPP
#define QAT_AST_GLOBAL_DECLARATION_HPP

#include "../IR/context.hpp"
#include "./expression.hpp"
#include "./node.hpp"
#include "./node_type.hpp"
#include "./types/qat_type.hpp"
#include <optional>

namespace qat::ast {

/**
 *  This AST element handles declaring Global variables in the language
 *
 * Currently initialisation is handled by `dyn_cast`ing the value to a constant
 * as that seems to be the proper way to do it.
 *
 */
class GlobalDeclaration : public Node {
private:
  String name;

  Maybe<QatType *> type;

  Expression *value;

  bool isVariable;

public:
  GlobalDeclaration(String _name, Maybe<QatType *> _type, Expression *_value,
                    bool _isVariable, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::globalDeclaration; }
};

} // namespace qat::ast

#endif