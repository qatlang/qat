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
  String   name;
  QatType *type;
  bool     variability;
  u64      argIndex;
  bool     isMemberForConstructor;

  // Construct a new Argument
  Argument(String _name, QatType *_type, bool _variability, u64 _arg_index,
           bool isMember)
      : name(std::move(_name)), type(_type), variability(_variability),
        argIndex(_arg_index), isMemberForConstructor(isMember) {}

public:
  // This constructs an immutable argument
  useit static Argument Create(const String &name, QatType *type,
                               u64 arg_index) {
    return {name, type, false, arg_index, false};
  }

  // This constructs an implicit member argument for constructors
  useit static Argument CreateMember(const String &name, QatType *type,
                                     u64 argIndex) {
    return {name, nullptr, true, argIndex, true};
  }

  // This constructs a variable argument
  useit static Argument CreateVariable(const String &name, QatType *type,
                                       u64 arg_index) {
    return {name, type, true, arg_index, false};
  }

  // Get the name of the argument
  useit String getName() const { return name; }

  // Get the LLVM type of the argument
  useit QatType *getType() const { return type; }

  // Get the variability of the argument
  useit bool get_variability() const { return variability; }

  // Get the index of the argument
  useit u64 getArgIndex() const { return argIndex; }

  useit bool isMemberArg() const { return isMemberForConstructor; }
};
} // namespace qat::IR

#endif