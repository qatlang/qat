#ifndef QAT_AST_ERRORS_HPP
#define QAT_AST_ERRORS_HPP

#include "../cli/color.hpp"
#include "../utils/file_range.hpp"
#include <iostream>

namespace qat::ast {

class Errors {
private:
  static void displayPlacement(unsigned number, utils::FileRange fileRange);
  static void displayLink(unsigned number);

public:
  // EAST0 - Type of LHS and RHS of assignment does not match
  static void AST0(std::string type1, std::string type2,
                   utils::FileRange fileRange);

  // EAST1 - Local value is not a variable
  static void AST1(std::string name, utils::FileRange fileRange);

  // EAST2 - Global or Static value is not a variable
  static void AST2(bool is_global, std::string name,
                   utils::FileRange fileRange);

  // EAST3 - Value cannot be found in the current scope or in any parent scopes
  static void AST3(std::string name, std::string function,
                   utils::FileRange fileRange);

  // EAST4 - The provided box is already exposed in a bigger scope
  static void AST4(std::string name, bool is_multiple,
                   utils::FileRange fileRange);
};

} // namespace qat::ast

#endif