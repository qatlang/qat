#include "./errors.hpp"

#define COLOR(col, value) colors::col << value << colors::reset
#define COLORBOLD(col, value) colors::bold::col << value << colors::reset
#define HIGHLIGHT(value) colors::bold::yellow << value << colors::reset

namespace qat::AST {

void Errors::displayPlacement(unsigned number, utils::FileRange placement) {
  std::cout << COLOR(red, "EAST" + std::to_string(number) + " => ")
            << COLORBOLD(green, placement.file.string()
                                    << ":" << placement.start.line << ":"
                                    << placement.start.character << " - "
                                    << placement.end.line << ":"
                                    << placement.end.character)
            << "\n   ";
}

void Errors::displayLink(unsigned number) {
  std::cout << "\n\nTo find out more, or for solutions, visit "
            << COLORBOLD(white, "https://qat.dev/errors/EAST" << number)
            << std::endl;
}

void Errors::AST0(std::string type1, std::string type2,
                  utils::FileRange placement) {
  displayPlacement(0, placement);
  std::cout << "Expression on the Left side of the assignment is of the type "
            << HIGHLIGHT(type1)
            << ", but the expression on the right side of the assignment is of "
               "the type "
            << HIGHLIGHT(type2);
  displayLink(0);
  exit(1);
}

void Errors::AST1(std::string name, utils::FileRange placement) {
  displayPlacement(1, placement);
  std::cout << "The local value " << HIGHLIGHT(name)
            << " is not a variable and cannot be reassigned";
  displayLink(1);
  exit(1);
}

void Errors::AST2(bool is_global, std::string name,
                  utils::FileRange placement) {
  displayPlacement(2, placement);
  std::cout << "The " << (is_global ? "global" : "static") << " value "
            << HIGHLIGHT(name) << " is not a variable and cannot be reassigned";
  displayLink(2);
  exit(1);
}

void Errors::AST3(std::string name, std::string function,
                  utils::FileRange placement) {
  displayPlacement(3, placement);
  std::cout << "The name " << HIGHLIGHT(name) << " not found in function "
            << HIGHLIGHT(function) << " and is not global or static value";
  displayLink(3);
  exit(1);
}

void Errors::AST4(std::string name, bool is_multiple,
                  utils::FileRange placement) {
  displayPlacement(4, placement);
  std::cout << "Box " << HIGHLIGHT(name)
            << " is already exposed. Please remove this box"
            << (is_multiple ? " from the list" : "");
  displayLink(4);
  exit(1);
}

} // namespace qat::AST