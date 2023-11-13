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
  Identifier            name;
  QatType*              type;
  Expression*           value;
  bool                  isVariable;
  Maybe<VisibilitySpec> visibSpec;

  mutable IR::GlobalEntity* globalEntity = nullptr;

public:
  GlobalDeclaration(Identifier _name, QatType* _type, Expression* _value, bool _isVariable,
                    Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange);

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::globalDeclaration; }
};

} // namespace qat::ast

#endif