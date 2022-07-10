#ifndef QAT_IR_ARGUMENT_HPP
#define QAT_IR_ARGUMENT_HPP

#include "types/qat_type.hpp"

#include <string>

namespace qat {
namespace IR {

/**
 *  Argument represents an argument of a function
 *
 */
class Argument {
  /**
   *  Name of the argument
   *
   */
  std::string name;

  /**
   *  Type of the argument
   *
   */
  QatType *type;

  /**
   *  Variability of the argument
   *
   */
  bool variability;

  /**
   *  Index of the argument in the function
   *
   */
  unsigned arg_index;

  /**
   *  Construct a new Argument
   *
   * @param _name Name of the argument
   * @param _type LLVM Type of the argument
   * @param _variability Variability of the argument
   * @param _arg_index Index of the argument in the function
   */
  Argument(std::string _name, QatType *_type, bool _variability,
           unsigned _arg_index)
      : name(_name), type(_type), variability(_variability),
        arg_index(_arg_index) {}

public:
  /**
   *  This constructs an immutable argument
   *
   * @param name Name of the argument
   * @param type LLVM Type of the argument
   * @param arg_index Index of the argument in the function
   * @return Argument
   */
  static Argument Create(std::string name, QatType *type, unsigned arg_index) {
    return Argument(name, type, false, arg_index);
  }

  /**
   *  This constructs a variable argument
   *
   * @param name Name of the argument
   * @param type LLVM Type of the argument
   * @param arg_index Index of the argument in the function
   * @return Argument
   */
  static Argument CreateVariable(std::string name, QatType *type,
                                 unsigned arg_index) {
    return Argument(name, type, true, arg_index);
  }

  /**
   *  Get the name of the argument
   *
   * @return std::string
   */
  std::string get_name() const { return name; }

  /**
   *  Get the LLVM type of the argument
   *
   * @return llvm::Type*
   */
  QatType *getType() const { return type; }

  /**
   *  Get the variability of the argument
   *
   * @return true
   * @return false
   */
  bool get_variability() const { return variability; }
};
} // namespace IR
} // namespace qat

#endif