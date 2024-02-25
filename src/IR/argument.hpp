#ifndef QAT_IR_ARGUMENT_HPP
#define QAT_IR_ARGUMENT_HPP

#include "../utils/identifier.hpp"

namespace qat::IR {

/**
 *  Argument represents an argument of a function
 *
 */
class Argument {
  Identifier name;
  QatType*   type;
  bool       variability;
  u64        argIndex;
  bool       isMemberForConstructor;
  bool       isReturnArg;

  // Construct a new Argument
  Argument(Identifier _name, QatType* _type, bool _variability, u64 _arg_index, bool isMember, bool _isReturnArg)
      : name(std::move(_name)), type(_type), variability(_variability), argIndex(_arg_index),
        isMemberForConstructor(isMember), isReturnArg(_isReturnArg) {}

public:
  // This constructs an immutable argument
  useit static Argument Create(const Identifier& name, QatType* type, u64 arg_index) {
    return {name, type, false, arg_index, false, false};
  }

  // This constructs an implicit member argument for constructors
  useit static Argument CreateMember(const Identifier& name, QatType* type, u64 argIndex) {
    return {name, type, true, argIndex, true, false};
  }

  // This constructs a variable argument
  useit static Argument CreateVariable(const Identifier& name, QatType* type, u64 arg_index) {
    return {name, type, true, arg_index, false, false};
  }

  useit static Argument CreateReturnArg(const String& name, QatType* type, u64 argIndex) {
    return {Identifier(name, fs::path("")), type, false, argIndex, false, true};
  }

  // Get the name of the argument
  useit Identifier getName() const { return name; }

  // Get the LLVM type of the argument
  useit QatType* getType() const { return type; }

  // Get the variability of the argument
  useit bool get_variability() const { return variability; }

  // Get the index of the argument
  useit u64 getArgIndex() const { return argIndex; }

  useit bool isMemberArg() const { return isMemberForConstructor; }

  useit bool isRetArg() const { return isReturnArg; }
};
} // namespace qat::IR

#endif