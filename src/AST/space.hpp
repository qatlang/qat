/**
 * Qat Programming Language : Copyright 2022 : Aldrin Mathew
 *
 * AAF INSPECTABLE LICENSE - 1.0
 *
 * This project is licensed under the AAF Inspectable License 1.0.
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
 * Inspectable License 1.0 available at this URL:
 *
 * https://github.com/aldrinsartfactory/InspectableLicense/
 *
 * This project may contain parts that are not licensed under the
 * same license. If so, the licenses of those parts should be
 * appropriately mentioned in those parts of the project. The
 * Author MAY provide a notice about the parts of the project that
 * are not licensed under the same license in a publicly visible
 * manner.
 *
 * You are NOT ALLOWED to sell, or distribute THIS project, it's
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

#ifndef QAT_AST_SPACE_HPP
#define QAT_AST_SPACE_HPP

#include <string>
#include <vector>

namespace qat {
namespace AST {
/**
 * @brief Space is a container for objects in the
 * QAT language. It is not a structural container,
 * but exists merely to avoid conflict between
 * libraries
 *
 */
class Space {
public:
  Space(std::string _name, Space *_parent, bool _isOpen)
      : name(_name), parent(_parent), isOpen(_isOpen) {}

  /**
   * @brief Name of the space. This is then appended to the
   * names of all members present in this space.
   *
   */
  std::string name;

  /**
   * @brief If this space has a parent space, then this will
   * not be null
   *
   */
  Space *parent;

  /**
   * @brief Whether this space is Open or not depending on the
   * context of the Parser. If this is open, then subsequent
   * members will be part of this spave
   *
   */
  bool isOpen = true;

  /**
   * @brief Resolve the depth of this space. This depends on the
   * parent space of this space and parent of that space and so on.
   *
   * @return std::vector<std::string> Returns the vector of names
   * of all spaces that this space is nested inside.
   */
  std::vector<std::string> resolve();

  /**
   * @brief Generates the complete value of the name of this space with the
   * names of all parent spaces prepended to this space's name
   *
   * @return std::string The complete value of the name of this space
   */
  std::string generate();

  /**
   * @brief Closes the space if it is open. A space is open on creation by
   * default. And being open ensures that all subsequent members are part
   * of this space
   *
   */
  void closeSpace();

  /**
   * @brief Whether this space has a parent space
   *
   * @return true if there is a parent
   * @return false if there is no parent
   */
  bool hasParent();
};
} // namespace AST
} // namespace qat

#endif