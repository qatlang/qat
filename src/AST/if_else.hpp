#ifndef QAT_AST_IF_ELSE_HPP
#define QAT_AST_IF_ELSE_HPP

#include "../utils/llvm_type_to_name.hpp"
#include "./block.hpp"
#include "./expression.hpp"
#include "./node_type.hpp"
#include "./sentence.hpp"
#include <optional>
#include <vector>

namespace qat {
namespace AST {

/**
 *  IfElse is used to represent two kinds of conditional sentences : If
 * and If-Else. The Else block is optional and if omitted, IfElse becomes a
 * plain if sentence
 *
 */
class IfElse : public Sentence {
private:
  /**
   *  Condition for this if sentence
   *
   */
  Expression *condition;

  /**
   *  The block belonging to the if section
   *
   */
  Block *if_block;

  /**
   *  The optional block belonging to the else section
   *
   */
  std::optional<Block *> else_block;

  /**
   *  The block after the if-else case
   *
   */
  Block *merge_block;

public:
  /**
   *  IfElse is used to represent two kinds of conditional statements : If
   * and If-Else. The Else block is optional and if omitted, this becomes a
   * plain if sentence
   *
   * @param _condition Condition in the if sentence
   * @param _if_block The Block of the if sentence
   * @param _else_block The optional Block of the else sentence
   * @param _merge_block The optional Block representing the remaining sentences
   * not part of the conditional blocks
   * @param _filePlacement
   *
   */
  IfElse(Expression *_condition, Block *_if_block,
         std::optional<Block *> _else_block,
         std::optional<Block *> _merge_block,
         utils::FilePlacement _filePlacement);

  llvm::Value *generate(IR::Generator *generator);

  backend::JSON toJSON() const;

  NodeType nodeType() { return NodeType::ifElse; }
};
} // namespace AST
} // namespace qat

#endif