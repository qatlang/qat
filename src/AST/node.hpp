#ifndef QAT_AST_NODE_HPP
#define QAT_AST_NODE_HPP

#include "../IR/generator.hpp"
#include "../utils/file_placement.hpp"
#include "./errors.hpp"
#include "node_type.hpp"
#include "llvm/IR/Value.h"

namespace qat {
namespace AST {

/**
 * @brief Node is the base class for all AST members of the language and it
 * requires a FilePlacement instance that indicates its position in the
 * corresponding file
 *
 */
class Node {
public:
  /**
   * @brief Node is the base class for all AST members of the language and it
   * requires a FilePlacement instance that indicates its position in the
   * corresponding file
   *
   * @param _filePlacement FilePlacement instance that represents the range
   * spanned by the tokens that made up this AST member
   */
  Node(utils::FilePlacement _filePlacement) : file_placement(_filePlacement) {}

  virtual ~Node(){};

  /**
   * @brief This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Value*
   */
  virtual llvm::Value *generate(IR::Generator *generator){};

  /**
   * @brief Type of the node represented by this AST member
   *
   * @return NodeType
   */
  virtual NodeType nodeType(){};

  /**
   * @brief A range present in a file that represents the placement spanned by
   * the tokens that made up this AST member
   *
   */
  utils::FilePlacement file_placement;
};

} // namespace AST
} // namespace qat

#endif