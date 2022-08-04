#ifndef QAT_IR_ARGUMENT_HPP
#define QAT_IR_ARGUMENT_HPP

#include "types/qat_type.hpp"

#include <string>

namespace qat::IR {

/**
 *  Argument represents an argument of a function
 *
 */
class Argument {
  // Name of the argument
  String name;
  // Type of the argument
  QatType *type;

  // Variability of the argument
  bool variability;

  // Index of the argument in the function
  u64 argIndex;

  // Construct a new Argument
  Argument(String _name, QatType *_type, bool _variability, u64 _arg_index)
      : name(std::move(_name)), type(_type), variability(_variability),
        argIndex(_arg_index) {}

public:
  // This constructs an immutable argument
  useit static Argument Create(const String &name, QatType *type,
                               u64 arg_index) {
    return {std::move(name), type, false, arg_index};
  }

  // This constructs a variable argument
  useit static Argument CreateVariable(const String &name, QatType *type,
                                       u64 arg_index) {
    return {std::move(name), type, true, arg_index};
  }

  // Get the name of the argument
  useit String getName() const { return name; }

  // Get the LLVM type of the argument
  useit QatType *getType() const { return type; }

  // Get the variability of the argument
  useit bool get_variability() const { return variability; }

  // Get the index of the argument
  useit u64 getArgIndex() const { return argIndex; }
};
} // namespace qat::IR

#endif