#ifndef QAT_AST_GLOBAL_DECLARATION_HPP
#define QAT_AST_GLOBAL_DECLARATION_HPP

#include "../IR/generator.hpp"
#include "./expression.hpp"
#include "./node.hpp"
#include "./node_type.hpp"
#include "./types/qat_type.hpp"
#include "llvm/ADT/Optional.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace AST {
/**
 *  This AST element handles declaring Global variables in the language
 *
 * Currently initialisation is handled by `dyn_cast`ing the value to a constant
 * as that seems to be the proper way to do it.
 *
 */
class GlobalDeclaration : public Node {
private:
  std::string name;
  llvm::Optional<Box> parent_space;
  llvm::Optional<QatType> type;
  Expression &value;
  bool is_variable;

public:
  GlobalDeclaration(std::string _name, llvm::Optional<Box> _parentSpace,
                    llvm::Optional<QatType> _type, Expression _value,
                    bool _isVariable, utils::FilePlacement _filePlacement);

  llvm::Value *generate(IR::Generator *generator);

  NodeType nodeType();
};
} // namespace AST
} // namespace qat

#endif