#include "./errors.hpp"

#define COLOR(col, value) colors::col << value << colors::reset
#define COLORBOLD(col, value) colors::bold::col << value << colors::reset
#define HIGHLIGHT(value) colors::bold::yellow << value << colors::reset

namespace qat::ast {

void Errors::displayPlacement(unsigned number, utils::FileRange fileRange) {
  std::cout << COLOR(red, "EAST" + std::to_string(number) + " => ")
            << COLORBOLD(green, fileRange.file.string()
                                    << ":" << fileRange.start.line << ":"
                                    << fileRange.start.character << " - "
                                    << fileRange.end.line << ":"
                                    << fileRange.end.character)
            << "\n   ";
}

void Errors::displayLink(unsigned number) {
  std::cout << "\n\nTo find out more, or for solutions, visit "
            << COLORBOLD(white, "https://qat.dev/errors/EAST" << number)
            << std::endl;
}

void Errors::AST0(std::string type1, std::string type2,
                  utils::FileRange fileRange) {
  displayPlacement(0, fileRange);
  std::cout << "Expression on the Left side of the assignment is of the type "
            << HIGHLIGHT(type1)
            << ", but the expression on the right side of the assignment is of "
               "the type "
            << HIGHLIGHT(type2);
  displayLink(0);
  exit(1);
}

void Errors::AST1(std::string name, utils::FileRange fileRange) {
  displayPlacement(1, fileRange);
  std::cout << "The local value " << HIGHLIGHT(name)
            << " is not a variable and cannot be reassigned";
  displayLink(1);
  exit(1);
}

void Errors::AST2(bool is_global, std::string name,
                  utils::FileRange fileRange) {
  displayPlacement(2, fileRange);
  std::cout << "The " << (is_global ? "global" : "static") << " value "
            << HIGHLIGHT(name) << " is not a variable and cannot be reassigned";
  displayLink(2);
  exit(1);
}

void Errors::AST3(std::string name, std::string function,
                  utils::FileRange fileRange) {
  displayPlacement(3, fileRange);
  std::cout << "The name " << HIGHLIGHT(name) << " not found in function "
            << HIGHLIGHT(function) << " and is not global or static value";
  displayLink(3);
  exit(1);
}

void Errors::AST4(std::string name, bool is_multiple,
                  utils::FileRange fileRange) {
  displayPlacement(4, fileRange);
  std::cout << "Box " << HIGHLIGHT(name)
            << " is already exposed. Please remove this box"
            << (is_multiple ? " from the list" : "");
  displayLink(4);
  exit(1);
}

} // namespace qat::ast