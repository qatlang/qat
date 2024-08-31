#include "./array.hpp"
#include "../context.hpp"
#include "../value.hpp"
#include "./qat_type.hpp"
#include "reference.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::ir {

ArrayType::ArrayType(Type* _element_type, u64 _length, llvm::LLVMContext& llctx)
    : elementType(_element_type), length(_length) {
  llvmType    = llvm::ArrayType::get(elementType->get_llvm_type(), length);
  linkingName = "qat'array:[" + elementType->get_name_for_linking() + "," + std::to_string(length) + "]";
}

ArrayType* ArrayType::get(Type* elementType, u64 _length, llvm::LLVMContext& llctx) {
  for (auto* typ : allTypes) {
    if (typ->is_array()) {
      if (typ->as_array()->get_length() == _length) {
        if (typ->as_array()->get_element_type()->is_same(elementType)) {
          return typ->as_array();
        }
      }
    }
  }
  return new ArrayType(elementType, _length, llctx);
}

Type* ArrayType::get_element_type() { return elementType; }

u64 ArrayType::get_length() const { return length; }

TypeKind ArrayType::type_kind() const { return TypeKind::array; }

String ArrayType::to_string() const { return "[" + std::to_string(length) + "]" + elementType->to_string(); }

Maybe<String> ArrayType::to_prerun_generic_string(ir::PrerunValue* val) const {
  if (can_be_prerun_generic()) {
    String result("[");
    for (usize i = 0; i < length; i++) {
      auto elemStr = elementType->to_prerun_generic_string(ir::PrerunValue::get(
          llvm::cast<llvm::ConstantArray>(val->get_llvm_constant())->getAggregateElement(i), elementType));
      if (elemStr.has_value()) {
        result += elemStr.value();
      } else {
        return None;
      }
      if (i != (length - 1)) {
        result += ", ";
      }
    }
    result += "]";
    return result;
  } else {
    return None;
  }
}

Maybe<bool> ArrayType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
  if (can_be_prerun_generic()) {
    if (first->get_ir_type()->is_same(second->get_ir_type())) {
      auto* array1 = llvm::cast<llvm::ConstantArray>(first->get_llvm_constant());
      auto* array2 = llvm::cast<llvm::ConstantArray>(second->get_llvm_constant());
      for (usize i = 0; i < length; i++) {
        if (!(elementType
                  ->equality_of(irCtx, ir::PrerunValue::get(array1->getAggregateElement(i), elementType),
                                ir::PrerunValue::get(array2->getAggregateElement(i), elementType))
                  .value())) {
          return false;
        }
      }
      return true;
    } else {
      return false;
    }
  } else {
    return None;
  }
}

bool ArrayType::is_type_sized() const { return length > 0; }

bool ArrayType::is_copy_constructible() const { return elementType->is_copy_constructible(); }

bool ArrayType::is_move_constructible() const { return elementType->is_move_constructible(); }

bool ArrayType::is_copy_assignable() const { return elementType->is_copy_assignable(); }

bool ArrayType::is_move_assignable() const { return elementType->is_move_assignable(); }

bool ArrayType::is_destructible() const { return elementType->is_destructible(); }

ir::PrerunValue* ArrayType::get_prerun_default_value(ir::Ctx* irCtx) {
  if (elementType->has_prerun_default_value()) {
    Vec<llvm::Constant*> elems;
    for (usize i = 0; i < length; i++) {
      elems.push_back(elementType->get_prerun_default_value(irCtx)->get_llvm());
    }
    return ir::PrerunValue::get(llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(this->get_llvm_type()), elems),
                                this);
  }
  return nullptr;
}

void ArrayType::copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
  if (is_copy_constructible()) {
    auto* Ty64Int = llvm::Type::getInt64Ty(irCtx->llctx);
    auto* firstIndexing =
        irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), first->get_llvm(),
                                         {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    auto* secondIndexing =
        irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), second->get_llvm(),
                                         {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    for (usize i = 0; i < length; i++) {
      if (i != 0) {
        firstIndexing  = irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), firstIndexing,
                                                          {llvm::ConstantInt::get(Ty64Int, 1u)});
        secondIndexing = irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), secondIndexing,
                                                          {llvm::ConstantInt::get(Ty64Int, 1u)});
      }
      elementType->copy_construct_value(
          irCtx, ir::Value::get(firstIndexing, ir::ReferenceType::get(true, elementType, irCtx), false),
          ir::Value::get(secondIndexing, ir::ReferenceType::get(true, elementType, irCtx), false), fun);
    }
  } else {
    irCtx->Error("Could not copy construct an instance of type " + irCtx->color(to_string()), None);
  }
}
void ArrayType::copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
  if (is_copy_assignable()) {
    auto* Ty64Int = llvm::Type::getInt64Ty(irCtx->llctx);
    auto* firstIndexing =
        irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), first->get_llvm(),
                                         {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    auto* secondIndexing =
        irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), second->get_llvm(),
                                         {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    for (usize i = 0; i < length; i++) {
      if (i != 0) {
        firstIndexing  = irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), firstIndexing,
                                                          {llvm::ConstantInt::get(Ty64Int, 1u)});
        secondIndexing = irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), secondIndexing,
                                                          {llvm::ConstantInt::get(Ty64Int, 1u)});
      }
      elementType->copy_assign_value(
          irCtx, ir::Value::get(firstIndexing, ir::ReferenceType::get(true, elementType, irCtx), false),
          ir::Value::get(secondIndexing, ir::ReferenceType::get(true, elementType, irCtx), false), fun);
    }
  } else {
    irCtx->Error("Could not copy assign an instance of type " + irCtx->color(to_string()), None);
  }
}

void ArrayType::move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
  if (is_move_assignable()) {
    auto* Ty64Int = llvm::Type::getInt64Ty(irCtx->llctx);
    auto* firstIndexing =
        irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), first->get_llvm(),
                                         {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    auto* secondIndexing =
        irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), second->get_llvm(),
                                         {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    for (usize i = 0; i < length; i++) {
      if (i != 0) {
        firstIndexing  = irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), firstIndexing,
                                                          {llvm::ConstantInt::get(Ty64Int, 1u)});
        secondIndexing = irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), secondIndexing,
                                                          {llvm::ConstantInt::get(Ty64Int, 1u)});
      }
      elementType->move_construct_value(
          irCtx, ir::Value::get(firstIndexing, ir::ReferenceType::get(true, elementType, irCtx), false),
          ir::Value::get(secondIndexing, ir::ReferenceType::get(true, elementType, irCtx), false), fun);
    }
  } else {
    irCtx->Error("Could not move construct an instance of type " + irCtx->color(to_string()), None);
  }
}

void ArrayType::move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
  if (is_move_assignable()) {
    auto* Ty64Int = llvm::Type::getInt64Ty(irCtx->llctx);
    auto* firstIndexing =
        irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), first->get_llvm(),
                                         {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    auto* secondIndexing =
        irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), second->get_llvm(),
                                         {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    for (usize i = 0; i < length; i++) {
      if (i != 0) {
        firstIndexing  = irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), firstIndexing,
                                                          {llvm::ConstantInt::get(Ty64Int, 1u)});
        secondIndexing = irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), secondIndexing,
                                                          {llvm::ConstantInt::get(Ty64Int, 1u)});
      }
      elementType->move_assign_value(
          irCtx, ir::Value::get(firstIndexing, ir::ReferenceType::get(true, elementType, irCtx), false),
          ir::Value::get(secondIndexing, ir::ReferenceType::get(true, elementType, irCtx), false), fun);
    }
  } else {
    irCtx->Error("Could not move assign an instance of type " + irCtx->color(to_string()), None);
  }
}

void ArrayType::destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
  if (elementType->is_destructible()) {
    auto* Ty64Int = llvm::Type::getInt64Ty(irCtx->llctx);
    auto* instanceIndexing =
        irCtx->builder.CreateInBoundsGEP(get_llvm_type(), instance->get_llvm(),
                                         {llvm::ConstantInt::get(Ty64Int, 0u), llvm::ConstantInt::get(Ty64Int, 0u)});
    for (usize i = 0; i < length; i++) {
      if (i != 0) {
        instanceIndexing = irCtx->builder.CreateInBoundsGEP(elementType->get_llvm_type(), instanceIndexing,
                                                            {llvm::ConstantInt::get(Ty64Int, 1u)});
      }
      elementType->destroy_value(
          irCtx, ir::Value::get(instanceIndexing, ir::ReferenceType::get(true, elementType, irCtx), false), fun);
    }
  } else {
    irCtx->Error("Could not destroy instance of type " + irCtx->color(to_string()), None);
  }
}

} // namespace qat::ir