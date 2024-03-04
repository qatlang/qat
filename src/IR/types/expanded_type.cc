#include "./expanded_type.hpp"
#include "../generics.hpp"
#include "../qat_module.hpp"
#include "./function.hpp"
#include "./reference.hpp"

namespace qat::ir {

ExpandedType::ExpandedType(Identifier _name, Vec<GenericParameter*> _generics, ir::Mod* _parent,
                           const VisibilityInfo& _visib)
    : name(std::move(_name)), generics(_generics), parent(_parent), visibility(_visib) {}

bool ExpandedType::is_generic() const { return !generics.empty(); }

bool ExpandedType::has_generic_parameter(const String& name) const {
  for (auto* gen : generics) {
    if (gen->is_same(name)) {
      return true;
    }
  }
  return false;
}

GenericParameter* ExpandedType::get_generic_parameter(const String& name) const {
  for (auto* gen : generics) {
    if (gen->is_same(name)) {
      return gen;
    }
  }
  return nullptr;
}

String ExpandedType::get_full_name() const {
  auto result = parent->get_fullname_with_child(name.value);
  if (is_generic()) {
    result += ":[";
    for (usize i = 0; i < generics.size(); i++) {
      result += generics.at(i)->to_string();
      if (i != (generics.size() - 1)) {
        result += ", ";
      }
    }
    result += "]";
  }
  return result;
}

Identifier ExpandedType::get_name() const { return name; }

Maybe<Method*> ExpandedType::check_valued_function(const Vec<Method*>& valuedMemberFunctions, const String& name) {
  for (auto* vFn : valuedMemberFunctions) {
    if (vFn->get_name().value == name) {
      return vFn;
    }
  }
  return None;
}

bool ExpandedType::has_valued_method(String const& name) const {
  return check_valued_function(valuedMemberFunctions, name).has_value();
}

Method* ExpandedType::get_valued_method(String const& name) const {
  return check_valued_function(valuedMemberFunctions, name).value();
}

Maybe<Method*> ExpandedType::check_normal_method(const Vec<Method*>& memberFunctions, const String& name) {
  for (auto* mFn : memberFunctions) {
    if (!mFn->is_variation_method() && mFn->get_name().value == name) {
      return mFn;
    }
  }
  return None;
}

bool ExpandedType::has_normal_method(const String& fnName) const {
  return check_normal_method(memberFunctions, fnName).has_value();
}

Maybe<Method*> ExpandedType::check_variation(Vec<Method*> const& variationFunctions, const String& name) {
  for (auto* mFn : variationFunctions) {
    if (mFn->is_variation_method() && mFn->get_name().value == name) {
      return mFn;
    }
  }
  return None;
}

bool ExpandedType::has_variation(String const& fnName) const {
  return check_variation(memberFunctions, fnName).has_value();
}

Method* ExpandedType::get_normal_method(const String& fnName) const {
  return check_normal_method(memberFunctions, fnName).value();
}

Method* ExpandedType::get_variation(const String& fnName) const {
  return check_variation(memberFunctions, fnName).value();
}

Maybe<ir::Method*> ExpandedType::check_static_method(Vec<ir::Method*> const& staticFns, const String& name) {
  for (const auto& fun : staticFns) {
    if (fun->get_name().value == name) {
      return fun;
    }
  }
  return None;
}

bool ExpandedType::has_static_method(const String& fnName) const {
  return check_static_method(staticFunctions, fnName).has_value();
}

Method* ExpandedType::get_static_method(const String& fnName) const {
  return check_static_method(staticFunctions, fnName).value();
}

Maybe<ir::Method*> ExpandedType::check_binary_operator(Vec<ir::Method*> const& binOps, const String& opr,
                                                       Pair<Maybe<bool>, ir::Type*> argType) {
  for (auto* bin : binOps) {
    if (bin->get_name().value == opr) {
      auto* binArgTy = bin->get_ir_type()->as_function()->get_argument_type_at(1)->get_type();
      if (binArgTy->is_same(argType.second) ||
          (argType.first.has_value() && binArgTy->is_reference() &&
           (binArgTy->as_reference()->isSubtypeVariable() ? argType.first.value() : true) &&
           binArgTy->as_reference()->get_subtype()->is_same(argType.second)) ||
          (argType.second->is_reference() && argType.second->as_reference()->get_subtype()->is_same(binArgTy))) {
        return bin;
      }
    }
  }
  return None;
}

bool ExpandedType::has_normal_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const {
  return check_binary_operator(normalBinaryOperators, opr, argType).has_value();
}

Method* ExpandedType::get_normal_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const {
  return check_binary_operator(normalBinaryOperators, opr, argType).value();
}

bool ExpandedType::has_variation_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const {
  return check_binary_operator(variationBinaryOperators, opr, argType).has_value();
}

Method* ExpandedType::get_variation_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const {
  return check_binary_operator(variationBinaryOperators, opr, argType).value();
}

Maybe<ir::Method*> ExpandedType::check_unary_operator(Vec<ir::Method*> const& unaryOperators, const String& opr) {
  for (auto* unr : unaryOperators) {
    if (unr->get_name().value == opr) {
      return unr;
    }
  }
  return None;
}

bool ExpandedType::has_unary_operator(const String& opr) const {
  return check_unary_operator(unaryOperators, opr).has_value();
}

Method* ExpandedType::get_unary_operator(const String& opr) const {
  return check_unary_operator(unaryOperators, opr).value();
}

bool ExpandedType::has_default_constructor() const { return defaultConstructor != nullptr; }

Method* ExpandedType::get_default_constructor() const { return defaultConstructor; }

bool ExpandedType::has_any_constructor() const { return (!constructors.empty()) || (defaultConstructor != nullptr); }

bool ExpandedType::has_any_from_convertor() const { return !fromConvertors.empty(); }

Maybe<ir::Method*> ExpandedType::check_constructor_with_types(Vec<ir::Method*> const&           cons,
                                                              Vec<Pair<Maybe<bool>, ir::Type*>> types) {
  for (auto* con : cons) {
    auto argTys = con->get_ir_type()->as_function()->get_argument_types();
    if (types.size() == (argTys.size() - 1)) {
      bool result = true;
      for (usize i = 1; i < argTys.size(); i++) {
        auto* argType = argTys.at(i)->get_type();
        if (!argType->is_same(types.at(i - 1).second) && !argType->isCompatible(types.at(i - 1).second) &&
            (!(types.at(i - 1).first.has_value() && argType->is_reference() &&
               (argType->as_reference()->isSubtypeVariable() ? types.at(i - 1).first.value() : true) &&
               (argType->as_reference()->get_subtype()->is_same(types.at(i - 1).second) ||
                argType->as_reference()->get_subtype()->isCompatible(types.at(i - 1).second)))) &&
            (!(types.at(i - 1).second->is_reference() &&
               (types.at(i - 1).second->as_reference()->get_subtype()->is_same(argType) ||
                types.at(i - 1).second->as_reference()->get_subtype()->isCompatible(argType))))) {
          result = false;
          break;
        }
      }
      if (result) {
        return con;
      }
    } else {
      continue;
    }
  }
  return None;
}

bool ExpandedType::has_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> argTypes) const {
  return check_constructor_with_types(constructors, argTypes).has_value();
}

Method* ExpandedType::get_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> argTypes) const {
  return check_constructor_with_types(constructors, argTypes).value();
}

Maybe<ir::Method*> ExpandedType::check_from_convertor(Vec<ir::Method*> const& fromConvs, Maybe<bool> isValueVar,
                                                      ir::Type* candTy) {
  for (auto* fconv : fromConvs) {
    auto* argTy = fconv->get_ir_type()->as_function()->get_argument_type_at(1)->get_type();
    if (argTy->is_same(candTy) || argTy->isCompatible(candTy) ||
        (isValueVar.has_value() && argTy->is_reference() &&
         (argTy->as_reference()->isSubtypeVariable() ? isValueVar.value() : true) &&
         (argTy->as_reference()->get_subtype()->is_same(candTy) ||
          argTy->as_reference()->get_subtype()->isCompatible(candTy))) ||
        (candTy->is_reference() && candTy->as_reference()->get_subtype()->is_same(argTy))) {
      return fconv;
    }
  }
  return None;
}

bool ExpandedType::has_from_convertor(Maybe<bool> isValueVar, ir::Type* candTy) const {
  return check_from_convertor(fromConvertors, isValueVar, candTy).has_value();
}

Method* ExpandedType::get_from_convertor(Maybe<bool> isValueVar, ir::Type* candTy) const {
  return check_from_convertor(fromConvertors, isValueVar, candTy).value();
}

Maybe<Method*> ExpandedType::check_to_convertor(Vec<Method*> const& toConvertors, ir::Type* targetTy) {
  for (auto* tconv : toConvertors) {
    auto* retTy = tconv->get_ir_type()->as_function()->get_return_type()->get_type();
    if (retTy->is_same(targetTy) ||
        (retTy->is_reference() && retTy->as_reference()->get_subtype()->is_same(targetTy)) ||
        (targetTy->is_reference() && targetTy->as_reference()->get_subtype()->is_same(retTy))) {
      return tconv;
    }
  }
  return None;
}

bool ExpandedType::has_to_convertor(ir::Type* typ) const {
  return ExpandedType::check_to_convertor(toConvertors, typ).has_value();
}

Method* ExpandedType::get_to_convertor(ir::Type* typ) const {
  return ExpandedType::check_to_convertor(toConvertors, typ).value();
}

bool ExpandedType::has_copy_constructor() const { return copyConstructor.has_value(); }

Method* ExpandedType::get_copy_constructor() const { return copyConstructor.value(); }

bool ExpandedType::has_move_constructor() const { return moveConstructor.has_value(); }

Method* ExpandedType::get_move_constructor() const { return moveConstructor.value(); }

bool ExpandedType::has_copy_assignment() const { return copyAssignment.has_value(); }

Method* ExpandedType::get_copy_assignment() const { return copyAssignment.value(); }

bool ExpandedType::has_move_assignment() const { return moveAssignment.has_value(); }

Method* ExpandedType::get_move_assignment() const { return moveAssignment.value(); }

bool ExpandedType::has_copy() const { return has_copy_constructor() && has_copy_assignment(); }

bool ExpandedType::has_move() const { return has_move_constructor() && has_move_assignment(); }

bool ExpandedType::has_destructor() const { return destructor.has_value(); }

ir::Method* ExpandedType::get_destructor() const { return destructor.value(); }

VisibilityInfo ExpandedType::get_visibility() const { return visibility; }

bool ExpandedType::is_accessible(const AccessInfo& reqInfo) const { return visibility.is_accessible(reqInfo); }

Mod* ExpandedType::get_module() { return parent; }

bool ExpandedType::is_expanded() const { return true; }

} // namespace qat::ir