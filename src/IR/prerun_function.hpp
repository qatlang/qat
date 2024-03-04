#ifndef QAT_IR_PRERUN_FUNCTION_HPP
#define QAT_IR_PRERUN_FUNCTION_HPP

#include "../utils/identifier.hpp"
#include "types/function.hpp"
#include "types/qat_type.hpp"
#include "value.hpp"

namespace qat::ir {

class PrerunFunction;

class PrerunLocal {
  Identifier   name;
  Type*        type = nullptr;
  bool         isVar;
  PrerunValue* value = nullptr;

  PrerunLocal(Identifier _name, Type* _type, bool _isVar, PrerunValue* _initialVal)
      : name(_name), type(_type), isVar(_isVar), value(_initialVal) {}

public:
  useit static inline PrerunLocal* get(Identifier _name, Type* _type, bool _isVar, PrerunValue* _initialVal) {
    return new PrerunLocal(_name, _type, _isVar, _initialVal);
  }

  useit inline Identifier   get_name() const { return name; }
  useit inline Type*        get_type() const { return type; }
  useit inline bool         is_variable() const { return isVar; }
  useit inline PrerunValue* get_value() const { return value; }

  void change_value(PrerunValue* other) { value = other; }
};

class PreBlock {
  PrerunFunction*   function = nullptr;
  Vec<PrerunLocal*> locals;
  PreBlock*         parent   = nullptr;
  PreBlock*         previous = nullptr;
  PreBlock*         next     = nullptr;

public:
  PreBlock(PrerunFunction* _function, PreBlock* _parent) : function(_function), parent(_parent) {}

  useit static inline PreBlock* get(PrerunFunction* _function, PreBlock* _parent) {
    return new PreBlock(_function, _parent);
  }

  useit bool has_previous() const { return previous != nullptr; }
  useit bool has_next() const { return next != nullptr; }
  useit bool has_parent() const { return parent != nullptr; }
  useit bool has_local(String const& name) {
    for (auto loc : locals) {
      if (loc->get_name().value == name) {
        return true;
      }
    }
    return false;
  }

  useit PrerunLocal* get_local(String const& name) {
    for (auto loc : locals) {
      if (loc->get_name().value == name) {
        return loc;
      }
    }
    return nullptr;
  }

  void set_previous(PreBlock* _prev) { previous = _prev; }
  void set_next(PreBlock* _next) { next = _next; }
};

class PreFnState {
  PrerunFunction* function = nullptr;
  Vec<PreBlock*>  blocks;
  usize           activeBlock = 0;

  PreFnState(PrerunFunction* _function) : function(_function) {}

public:
  useit static inline PreFnState* get(PrerunFunction* fun) { return new PreFnState(fun); }
};

class PrerunFunction : public PrerunValue {
  Identifier         name;
  Type*              returnType;
  Vec<ArgumentType*> argTypes;

public:
  PrerunFunction(Identifier _name, Type* _retTy, Vec<ArgumentType*> _argTys, llvm::LLVMContext& ctx)
      : PrerunValue(nullptr, new ir::FunctionType(ReturnType::get(_retTy), _argTys, ctx)), name(_name),
        returnType(_retTy), argTypes(_argTys) {}

  useit Identifier    get_name() const { return name; }
  useit Type*         get_return_type() const { return returnType; }
  useit ArgumentType* get_argument_at(usize index) { return argTypes[index]; }
};

} // namespace qat::ir

#endif