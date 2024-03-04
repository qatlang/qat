#ifndef QAT_IR_ARGUMENT_HPP
#define QAT_IR_ARGUMENT_HPP

#include "../utils/identifier.hpp"
#include "types/qat_type.hpp"

namespace qat::ir {

/**
 *  Argument represents an argument of a function
 *
 */
class Argument {
  Identifier name;
  Type*      type;
  bool       variability;
  u64        argIndex;
  bool       isMemberForConstructor;
  bool       isReturnArg;

  // Construct a new Argument
  Argument(Identifier _name, Type* _type, bool _variability, u64 _arg_index, bool isMember, bool _isReturnArg)
      : name(std::move(_name)), type(_type), variability(_variability), argIndex(_arg_index),
        isMemberForConstructor(isMember), isReturnArg(_isReturnArg) {}

public:
  // This constructs an immutable argument
  useit static Argument Create(const Identifier& name, Type* type, u64 arg_index) {
    return {name, type, false, arg_index, false, false};
  }

  // This constructs an implicit member argument for constructors
  useit static Argument CreateMember(const Identifier& name, Type* type, u64 argIndex) {
    return {name, type, true, argIndex, true, false};
  }

  // This constructs a variable argument
  useit static Argument CreateVariable(const Identifier& name, Type* type, u64 arg_index) {
    return {name, type, true, arg_index, false, false};
  }

  useit Identifier get_name() const { return name; }

  useit Type* get_type() const { return type; }
  useit bool  get_variability() const { return variability; }
  useit u64   get_arg_index() const { return argIndex; }
  useit bool  is_member_argument() const { return isMemberForConstructor; }
};
} // namespace qat::ir

#endif