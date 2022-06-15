#ifndef QAT_AST_ERRORS_HPP
#define QAT_AST_ERRORS_HPP

#include "../CLI/color.hpp"
#include "../utils/file_placement.hpp"
#include <iostream>

namespace qat {
namespace AST {

class Error {
private:
  static void displayPlacement(unsigned number, utils::FilePlacement placement);
  static void displayLink(unsigned number);

public:
  // EAST0 - Type of local value and the type of the value for assignment
  // doesn't match
  static void AST0(std::string name, std::string type1, std::string type2,
                   utils::FilePlacement placement);

  // EAST1 - Local value is not a variable
  static void AST1(std::string name, utils::FilePlacement placement);

  // EAST2 - Type of Global or Static value and the type of the value for
  // assignment doesn't match
  static void AST2(bool is_global, std::string name, std::string type1,
                   std::string type2, utils::FilePlacement placement);

  // EAST3 - Global or Static value is not a variable
  static void AST3(bool is_global, std::string name,
                   utils::FilePlacement placement);

  // EAST4 - Value cannot be found in the current scope or in any parent scopes
  static void AST4(std::string name, std::string function,
                   utils::FilePlacement placement);

  // EAST5 - The provided box is already exposed in a bigger scope
  static void AST5(std::string name, bool is_multiple,
                   utils::FilePlacement placement);
};

} // namespace AST
} // namespace qat

#endif