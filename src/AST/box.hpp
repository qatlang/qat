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

#ifndef QAT_AST_BOX_HPP
#define QAT_AST_BOX_HPP

#include <string>
#include <vector>

namespace qat {
namespace AST {
/**
 *  Box is a container for objects in the
 * QAT language. It is not a structural container,
 * but exists merely to avoid conflict between
 * libraries
 *
 */
class Box {
public:
  Box(std::string _name, Box *_parent, bool _isOpen)
      : name(_name), parent(_parent), isOpen(_isOpen) {}

  /**
   *  Name of the box. This is then appended to the
   * names of all members present in this box.
   *
   */
  std::string name;

  /**
   *  If this box has a parent box, then this will
   * not be nullptr
   *
   */
  Box *parent;

  /**
   *  Whether this box is Open or not depending on the
   * context of the Parser. If this is open, then subsequent
   * members will be part of this spave
   *
   */
  bool isOpen = true;

  /**
   *  Resolve the depth of this box. This depends on the
   * parent box of this box and parent of that box and so on.
   *
   * @return std::vector<std::string> Returns the vector of names
   * of all boxes that this box is nested inside.
   */
  std::vector<std::string> resolve();

  /**
   *  Generates the complete value of the name of this box with the
   * names of all parent boxes prepended to this box's name
   *
   * @return std::string The complete value of the name of this box
   */
  std::string generate();

  /**
   *  Closes the box if it is open. A box is open on creation by
   * default. And being open ensures that all subsequent members are part
   * of this box
   *
   */
  void close();

  /**
   *  Whether this box has a parent box
   *
   * @return true if there is a parent
   * @return false if there is no parent
   */
  bool has_parent();
};
} // namespace AST
} // namespace qat

#endif