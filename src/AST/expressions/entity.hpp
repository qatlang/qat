#ifndef QAT_AST_EXPRESSIONS_ENTITY_HPP
#define QAT_AST_EXPRESSIONS_ENTITY_HPP

#include "../../IR/generator.hpp"
#include "../../utils/pointer_kind.hpp"
#include "../../utils/variability.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include "llvm/IR/Value.h"
#include <string>

namespace qat {
namespace AST {

/**
 *  Entity represents either a variable or constant. The name of the
 * entity is mostly just an identifier, but it can be other values if the
 * entity is present in the global constant
 *
 */
class Entity : public Expression {

private:
  /**
   *  Name of the entity
   *
   */
  std::string name;

public:
  /**
   *  Entity represents either a variable or constant. The name of the
   * entity is mostly just an identifier, but it can be other values if the
   * entity is present in the global constant
   *
   * @param _name Name of the entity
   * @param _filePlacement FilePLacement instance that represents the range
   * spanned by the tokens making up this AST member
   */
  Entity(std::string _name, utils::FilePlacement _filePlacement)
      : name(_name), Expression(_filePlacement) {}

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
  NodeType nodeType() { return qat::AST::NodeType::entity; }
};

} // namespace AST
} // namespace qat

#endif