#ifndef QAT_IR_ARGUMENT_HPP
#define QAT_IR_ARGUMENT_HPP

#include "llvm/IR/Argument.h"
#include "llvm/IR/Type.h"
#include <string>

namespace qat {
namespace IR {

/**
 * @brief Argument represents an argument of a function
 *
 */
class Argument {
  /**
   * @brief Name of the argument
   *
   */
  std::string name;

  /**
   * @brief Type of the argument
   *
   */
  llvm::Type *type;

  /**
   * @brief Variability of the argument
   *
   */
  bool variability;

  /**
   * @brief Index of the argument in the function
   *
   */
  unsigned arg_index;

  /**
   * @brief Construct a new Argument
   *
   * @param _name Name of the argument
   * @param _type LLVM Type of the argument
   * @param _variability Variability of the argument
   * @param _arg_index Index of the argument in the function
   */
  Argument(std::string _name, llvm::Type *_type, bool _variability,
           unsigned _arg_index)
      : name(_name), type(_type), variability(_variability),
        arg_index(_arg_index) {}

public:
  /**
   * @brief This constructs an immutable argument
   *
   * @param name Name of the argument
   * @param type LLVM Type of the argument
   * @param arg_index Index of the argument in the function
   * @return Argument
   */
  static Argument Create(std::string name, llvm::Type *type,
                         unsigned arg_index) {
    return Argument(name, type, false, arg_index);
  }

  /**
   * @brief This constructs a variable argument
   *
   * @param name Name of the argument
   * @param type LLVM Type of the argument
   * @param arg_index Index of the argument in the function
   * @return Argument
   */
  static Argument CreateVariable(std::string name, llvm::Type *type,
                                 unsigned arg_index) {
    return Argument(name, type, true, arg_index);
  }

  /**
   * @brief Get the name of the argument
   *
   * @return std::string
   */
  std::string get_name() const { return name; }

  /**
   * @brief Get the LLVM type of the argument
   *
   * @return llvm::Type*
   */
  llvm::Type *get_type() const { return type; }

  /**
   * @brief Get the variability of the argument
   *
   * @return true
   * @return false
   */
  bool get_variability() const { return variability; }
};
} // namespace IR
} // namespace qat

#endif