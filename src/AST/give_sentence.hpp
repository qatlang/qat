#ifndef QAT_AST_GIVE_SENTENCE_HPP
#define QAT_AST_GIVE_SENTENCE_HPP

#include "../IR/generator.hpp"
#include "../utils/llvm_type_to_name.hpp"
#include "expression.hpp"
#include "sentence.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include <optional>

namespace qat {
namespace AST {
/**
 *  GiveSentence represents a return statement in the language
 *
 */
class GiveSentence : public Sentence {
private:
  /**
   *  Expression that should be returned/given
   *
   */
  std::optional<Expression *> give_expr;

public:
  /**
   *  GiveSentence represents a return statement in the language
   *
   * @param _filePlacement
   */
  GiveSentence(std::optional<Expression *> _given_expr,
               utils::FilePlacement _filePlacement);

  /**
   *  This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Value*
   */
  llvm::Value *generate(IR::Generator *generator);

  /**
   *  Type of the node represented by this AST member
   *
   * @return NodeType
   */
  NodeType nodeType() { return qat::AST::NodeType::giveSentence; }

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif