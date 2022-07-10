#ifndef QAT_AST_GLOBAL_DECLARATION_HPP
#define QAT_AST_GLOBAL_DECLARATION_HPP

#include "../IR/context.hpp"
#include "./expression.hpp"
#include "./node.hpp"
#include "./node_type.hpp"
#include "./types/qat_type.hpp"

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

  llvm::Optional<QatType *> type;

  Expression *value;

  bool isVariable;

public:
  GlobalDeclaration(std::string _name, llvm::Optional<QatType *> _type,
                    Expression *_value, bool _isVariable,
                    utils::FilePlacement _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return NodeType::globalDeclaration; }
};
} // namespace AST
} // namespace qat

#endif