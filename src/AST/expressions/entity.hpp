/**
 * Qat Programming Language : Copyright 2022 : Aldrin Mathew
 *
 * AAF INSPECTABLE LICENCE - 1.0
 *
 * This project is licensed under the AAF Inspectable Licence 1.0.
 * You are allowed to inspect the source of this project(s) free of
 * cost, and also to verify the authenticity of the product.
 *
 * Unless required by applicable law, this project is provided
 * "AS IS", WITHOUT ANY WARRANTIES OR PROMISES OF ANY KIND, either
 * expressed or implied. The author(s) of this project is not
 * liable for any harms, errors or troubles caused by using the
 * source or the product, unless implied by law. By using this
 * project, or part of it, you are acknowledging the complete terms
 * and conditions of licensing of this project as specified in AAF
 * Inspectable Licence 1.0 available at this URL:
 *
 * https://github.com/aldrinsartfactory/InspectableLicence/
 *
 * This project may contain parts that are not licensed under the
 * same licence. If so, the licences of those parts should be
 * appropriately mentioned in those parts of the project. The
 * Author MAY provide a notice about the parts of the project that
 * are not licensed under the same licence in a publicly visible
 * manner.
 *
 * You are NOT ALLOWED to sell, or distribute THIS project, its
 * contents, the source or the product or the build result of the
 * source under commercial or non-commercial purposes. You are NOT
 * ALLOWED to revamp, rebrand, refactor, modify, the source, product
 * or the contents of this project.
 *
 * You are NOT ALLOWED to use the name, branding and identity of this
 * project to identify or brand any other project. You ARE however
 * allowed to use the name and branding to pinpoint/show the source
 * of the contents/code/logic of any other project. You are not
 * allowed to use the identification of the Authors of this project
 * to associate them to other projects, in a way that is deceiving
 * or misleading or gives out false information.
 */

#ifndef QAT_AST_EXPRESSIONS_ENTITY_HPP
#define QAT_AST_EXPRESSIONS_ENTITY_HPP

#include "../../IR/generator.hpp"
#include "../../utils/variability.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"
#include "./reference_entity.hpp"
#include "llvm/IR/Value.h"
#include <string>

namespace qat {
namespace AST {

/**
 * @brief Entity represents either a variable or constant. The name of the
 * entity is mostly just an identifier, but it can be other values if the
 * entity is present in the global constant
 *
 */
class Entity : public Expression {

private:
  /**
   * @brief Name of the entity
   *
   */
  std::string name;

public:
  /**
   * @brief Entity represents either a variable or constant. The name of the
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
   * @brief Convert this entity to a ReferenceEntity
   *
   * @return ReferenceEntity
   */
  ReferenceEntity to_reference();

  /**
   * @brief This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Value*
   */
  llvm::Value *generate(IR::Generator *generator);

  /**
   * @brief Type of the node represented by this AST member
   *
   * @return NodeType
   */
  NodeType nodeType() { return qat::AST::NodeType::entity; }
};

} // namespace AST
} // namespace qat

#endif