#ifndef QAT_IR_PRERUN_FUNCTION_HPP
#define QAT_IR_PRERUN_FUNCTION_HPP

#include "../utils/identifier.hpp"
#include "types/function.hpp"
#include "types/qat_type.hpp"
#include "value.hpp"

namespace qat::ast {
class PrerunSentence;
}

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
  Vec<PreBlock*>    children;
  PreBlock*         parent   = nullptr;
  PreBlock*         previous = nullptr;
  PreBlock*         next     = nullptr;

  PreBlock(PrerunFunction* _function, PreBlock* _parent) : function(_function), parent(_parent) {
    if (parent) {
      parent->children.push_back(this);
    }
  }

public:
  ~PreBlock() {
    for (auto loc : locals) {
      delete loc;
    }
  }

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

  useit PrerunFunction* get_fn() const { return function; }

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

class PrerunCallState {
  friend class PrerunFunction;
  PrerunFunction*   function = nullptr;
  Vec<PrerunValue*> argumentValues;
  Vec<PreBlock*>    blocks;
  usize             activeBlock = 0;

  Maybe<PrerunValue*> givenValue;
  usize               emitNesting = 0;

  PrerunCallState(PrerunFunction* _function, Vec<PrerunValue*> _argVals)
      : function(_function), argumentValues(_argVals) {}

public:
  ~PrerunCallState() {
    for (auto blk : blocks) {
      delete blk;
    }
  }

  useit static inline PrerunCallState* get(PrerunFunction* fun, Vec<PrerunValue*> argVals) {
    return new PrerunCallState(fun, argVals);
  }

  useit inline PrerunFunction* get_function() const { return function; }
  useit bool                   has_arg_with_name(String const& name);
  useit PrerunValue*           get_arg_value_for(String const& name);

  void inline set_given_value(PrerunValue* _givenVal) { givenValue = _givenVal; }
  useit bool inline has_given_value() const { return givenValue.has_value(); }
  useit PrerunValue* get_given_value() const { return givenValue.value(); }

  void               increment_emit_nesting() { emitNesting++; }
  void               decrement_emit_nesting() { emitNesting--; }
  useit inline usize get_emit_nesting() const { return emitNesting; }
};

class PrerunFunction : public PrerunValue {
  friend class PrerunCallState;
  Identifier         name;
  Type*              returnType;
  Vec<ArgumentType*> argTypes;
  Mod*               parent;

  Pair<Vec<ast::PrerunSentence*>, FileRange> sentences;

public:
  PrerunFunction(Mod* _parent, Identifier _name, Type* _retTy, Vec<ArgumentType*> _argTys,
                 Pair<Vec<ast::PrerunSentence*>, FileRange> _sentences, llvm::LLVMContext& ctx)
      : PrerunValue(nullptr, new ir::FunctionType(ReturnType::get(_retTy), _argTys, ctx)), name(_name),
        returnType(_retTy), argTypes(_argTys), parent(_parent), sentences(_sentences) {}

  useit Identifier    get_name() const { return name; }
  useit Type*         get_return_type() const { return returnType; }
  useit ArgumentType* get_argument_type_at(usize index) { return argTypes[index]; }
  useit Mod*          get_module() const { return parent; }

  PrerunValue* call_prerun(Vec<PrerunValue*> arguments, Ctx* irCtx, FileRange fileRange);
};

} // namespace qat::ir

#endif