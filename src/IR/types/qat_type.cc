#include "./qat_type.hpp"
#include "../../show.hpp"
#include "./array.hpp"
#include "./choice.hpp"
#include "./definition.hpp"
#include "./flag.hpp"
#include "./float.hpp"
#include "./function.hpp"
#include "./future.hpp"
#include "./integer.hpp"
#include "./maybe.hpp"
#include "./mix.hpp"
#include "./native_type.hpp"
#include "./opaque.hpp"
#include "./pointer.hpp"
#include "./reference.hpp"
#include "./region.hpp"
#include "./result.hpp"
#include "./struct_type.hpp"
#include "./text.hpp"
#include "./tuple.hpp"
#include "./type_kind.hpp"
#include "./unsigned.hpp"
#include "./vector.hpp"

namespace qat::ir {

Type::Type() { allTypes.push_back(this); }

Type::~Type() {
	for (auto* skill : doneSkills) {
		std::destroy_at(skill);
	}
}

Vec<Type*> Type::allTypes = {};

Vec<Region*> Type::allRegions() {
	Vec<Region*> result;
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::REGION) {
			result.push_back(typ->as_region());
		}
	}
	return result;
}

void Type::clear_all() {
	for (auto* typ : allTypes) {
		std::destroy_at(typ);
	}
}

String Type::get_name_for_linking() const { return linkingName; }

bool Type::can_be_prerun_generic() const { return false; }

bool Type::can_be_prerun() const { return false; }

Maybe<String> Type::to_prerun_generic_string(ir::PrerunValue* val) const { return None; }

bool Type::is_type_sized() const { return false; }

Maybe<bool> Type::equality_of(ir::Ctx*, ir::PrerunValue* first, ir::PrerunValue* second) const { return None; }

bool Type::isCompatible(Type* other) {
	if (is_mark() && other->is_mark()) {
		if ((as_mark()->get_subtype()->is_same(other->as_mark()->get_subtype())) &&
		    (as_mark()->get_owner().is_same(other->as_mark()->get_owner()) ||
		     as_mark()->get_owner().is_of_anonymous() ||
		     (as_mark()->get_owner().is_of_any_region() && other->as_mark()->get_owner().is_of_region())) &&
		    (as_mark()->is_subtype_variable() ? other->as_mark()->is_subtype_variable() : true) &&
		    (as_mark()->is_non_nullable() ? other->as_mark()->is_non_nullable() : true) &&
		    (as_mark()->is_slice() == other->as_mark()->is_slice())) {
			return true;
		} else {
			return is_same(other);
		}
	} else if (is_ref() && other->is_ref()) {
		return (as_ref()->get_subtype()->is_same(other->as_ref()->get_subtype()) &&
		        (as_ref()->has_variability() ? other->as_ref()->has_variability() : true));
	} else {
		return is_same(other);
	}
}

bool Type::is_same(Type* other) {
	if (type_kind() != other->type_kind()) {
		if (type_kind() == TypeKind::DEFINITION) {
			return ((DefinitionType*)this)->get_subtype()->is_same(other);
		} else if (other->type_kind() == TypeKind::DEFINITION) {
			return ((DefinitionType*)other)->get_subtype()->is_same(this);
		} else if (type_kind() == TypeKind::OPAQUE) {
			if (((OpaqueType*)this)->has_subtype()) {
				return ((OpaqueType*)this)->get_subtype()->is_same(other);
			} else {
				return false;
			}
		} else if (other->type_kind() == TypeKind::OPAQUE) {
			if (((OpaqueType*)other)->has_subtype()) {
				return (((OpaqueType*)other)->get_subtype()->is_same(this));
			} else {
				return false;
			}
		}
		return false;
	} else {
		switch (type_kind()) {
			case TypeKind ::DEFINITION: {
				return ((DefinitionType*)this)->get_subtype()->is_same(((DefinitionType*)other)->get_subtype());
			}
			case TypeKind::OPAQUE: {
				auto* thisVal  = (OpaqueType*)this;
				auto* otherVal = (OpaqueType*)other;
				if (thisVal->has_subtype() && otherVal->has_subtype()) {
					return thisVal->get_subtype()->is_same(otherVal->get_subtype());
				} else {
					return thisVal->get_id() == otherVal->get_id();
				}
			}
			case TypeKind::MARK: {
				return (((MarkType*)this)->is_subtype_variable() == ((MarkType*)other)->is_subtype_variable()) &&
				       (((MarkType*)this)->is_nullable() == ((MarkType*)other)->is_nullable()) &&
				       (((MarkType*)this)->get_subtype()->is_same(((MarkType*)other)->get_subtype())) &&
				       (((MarkType*)this)->get_owner().is_same(((MarkType*)other)->get_owner()));
			}
			case TypeKind::REFERENCE: {
				return (((RefType*)this)->has_variability() == ((RefType*)other)->has_variability()) &&
				       (((RefType*)this)->get_subtype()->is_same(((RefType*)other)->get_subtype()));
			}
			case TypeKind::FUTURE: {
				auto* thisVal  = (FutureType*)this;
				auto* otherVal = (FutureType*)other;
				return thisVal->get_subtype()->is_same(otherVal->get_subtype()) &&
				       (thisVal->is_type_packed() == otherVal->is_type_packed());
			}
			case TypeKind::MAYBE: {
				auto* thisVal  = (MaybeType*)this;
				auto* otherVal = (MaybeType*)this;
				return thisVal->get_subtype()->is_same(otherVal->get_subtype()) &&
				       (thisVal->is_type_packed() == otherVal->is_type_packed());
			}
			case TypeKind::UNSIGNED_INTEGER: {
				return (((UnsignedType*)this)->get_bitwidth() == ((UnsignedType*)other)->get_bitwidth()) &&
				       (((UnsignedType*)this)->is_this_bool_type() == ((UnsignedType*)other)->is_this_bool_type());
			}
			case TypeKind::INTEGER: {
				return (((IntegerType*)this)->get_bitwidth() == ((IntegerType*)other)->get_bitwidth());
			}
			case TypeKind::FLOAT: {
				return (((FloatType*)this)->get_float_kind() == ((FloatType*)other)->get_float_kind());
			}
			case TypeKind::NATIVE: {
				auto* thisVal  = (NativeType*)this;
				auto* otherVal = (NativeType*)other;
				return thisVal->get_c_type_kind() == otherVal->get_c_type_kind() &&
				       (thisVal->is_c_ptr() ? (thisVal->get_subtype()->is_same(otherVal->get_subtype())) : true);
			}
			case TypeKind::TEXT: {
				auto* thisVal  = (TextType*)this;
				auto* otherVal = (TextType*)other;
				return thisVal->is_packed() == otherVal->is_packed();
			}
			case TypeKind::VOID:
			case TypeKind::TYPED: {
				return true;
			}
			case TypeKind::ARRAY: {
				auto* thisVal  = (ArrayType*)this;
				auto* otherVal = (ArrayType*)other;
				if (thisVal->get_length() == otherVal->get_length()) {
					return thisVal->get_element_type()->is_same(otherVal->get_element_type());
				} else {
					return false;
				}
			}
			case TypeKind::VECTOR: {
				auto* thisVal  = (VectorType*)this;
				auto* otherVal = (VectorType*)other;
				if (thisVal->get_count() == otherVal->get_count()) {
					return thisVal->get_element_type()->is_same(otherVal->get_element_type()) &&
					       (thisVal->get_vector_kind() == otherVal->get_vector_kind());
				} else {
					return false;
				}
			}
			case TypeKind::TUPLE: {
				auto* thisVal  = (TupleType*)this;
				auto* otherVal = (TupleType*)other;
				if ((thisVal->isPackedTuple() == otherVal->isPackedTuple()) &&
				    thisVal->getSubTypeCount() == otherVal->getSubTypeCount()) {
					for (usize i = 0; i < thisVal->getSubTypeCount(); i++) {
						if (not(thisVal->getSubtypeAt(i)->is_same(otherVal->getSubtypeAt(i)))) {
							return false;
						}
					}
					return true;
				} else {
					return false;
				}
			}
			case TypeKind::STRUCT: {
				auto* thisVal  = (StructType*)this;
				auto* otherVal = (StructType*)other;
				return thisVal->get_id() == otherVal->get_id();
			}
			case TypeKind::REGION: {
				auto* thisVal  = (Region*)this;
				auto* otherVal = (Region*)this;
				return thisVal->get_id() == otherVal->get_id();
			}
			case TypeKind::CHOICE: {
				auto* thisVal  = (ChoiceType*)this;
				auto* otherVal = (ChoiceType*)other;
				return thisVal->get_id() == otherVal->get_id();
			}
			case TypeKind::MIX: {
				auto* thisVal  = (MixType*)this;
				auto* otherVal = (MixType*)other;
				return thisVal->get_id() == otherVal->get_id();
			}
			case TypeKind::FLAG: {
				auto* thisVal  = (FlagType*)this;
				auto* otherVal = (FlagType*)other;
				return thisVal->get_id() == otherVal->get_id();
			}
			case TypeKind::RESULT: {
				auto* thisVal  = (ResultType*)this;
				auto* otherVal = (ResultType*)other;
				return (thisVal->isPacked == otherVal->isPacked) &&
				       thisVal->get_valid_type()->is_same(otherVal->get_valid_type()) &&
				       thisVal->get_error_type()->is_same(otherVal->get_error_type());
			}
			case TypeKind::FUNCTION: {
				auto* thisVal  = (FunctionType*)this;
				auto* otherVal = (FunctionType*)other;
				if (thisVal->get_argument_count() == otherVal->get_argument_count()) {
					if (thisVal->get_return_type()->get_type()->is_same(otherVal->get_return_type()->get_type()) &&
					    (thisVal->get_return_type()->is_return_self() ==
					     otherVal->get_return_type()->is_return_self())) {
						for (usize i = 0; i < thisVal->get_argument_count(); i++) {
							auto* thisArg  = thisVal->get_argument_type_at(i);
							auto* otherArg = otherVal->get_argument_type_at(i);
							if (not thisArg->is_same_as(otherArg)) {
								return false;
							}
						}
						return true;
					} else {
						return false;
					}
				} else {
					return false;
				}
			}
		}
	}
}

bool Type::is_expanded() const { return false; }

ExpandedType* Type::as_expanded() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? as_type_definition()->get_subtype()->as_expanded()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_expanded() : (ExpandedType*)this);
}

bool Type::is_typed() const {
	return (type_kind() == TypeKind::TYPED) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_opaque());
}

TypedType* Type::as_typed() const {
	return (type_kind() == TypeKind::DEFINITION) ? ((DefinitionType*)this)->get_subtype()->as_typed()
	                                             : (TypedType*)this;
}

bool Type::is_opaque() const {
	return (type_kind() == TypeKind::OPAQUE) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_opaque());
}

OpaqueType* Type::as_opaque() const {
	return (type_kind() == TypeKind::DEFINITION) ? ((DefinitionType*)this)->get_subtype()->as_opaque()
	                                             : (OpaqueType*)this;
}

bool Type::has_simple_copy() const { return false; }

bool Type::has_simple_move() const { return false; }

bool Type::has_prerun_default_value() const { return false; }

ir::PrerunValue* Type::get_prerun_default_value(ir::Ctx* irCtx) { return nullptr; }

bool Type::is_default_constructible() const { return has_prerun_default_value(); }

void Type::default_construct_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
	if (has_prerun_default_value()) {
		auto* defVal = get_prerun_default_value(irCtx);
		irCtx->builder.CreateStore(defVal->get_llvm(), instance->get_llvm());
	} else {
		irCtx->Error("Could not default construct an instance of type " + irCtx->color(to_string()), None);
	}
}

bool Type::is_copy_constructible() const { return has_simple_copy(); }

void Type::copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (not has_simple_copy()) {
		irCtx->Error("Could not copy construct an instance of type " + irCtx->color(to_string()) +
		                 " as it does not support simple-copy",
		             None);
	}
	irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
}

bool Type::is_copy_assignable() const { return has_simple_copy(); }

void Type::copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (not has_simple_copy()) {
		irCtx->Error("Could not copy assign an instance of type " + irCtx->color(to_string()) +
		                 " as it does not support simple-copy",
		             None);
	}
	irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
}

bool Type::is_move_constructible() const { return has_simple_move(); }

void Type::move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (has_simple_move()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(get_llvm_type()), second->get_llvm());
	} else {
		irCtx->Error("Could not move construct an instance of type " + irCtx->color(to_string()) +
		                 " as it does not support simple-move",
		             None);
	}
}

bool Type::is_move_assignable() const { return has_simple_move(); }

void Type::move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (has_simple_move()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(get_llvm_type()), second->get_llvm());
	} else {
		irCtx->Error("Could not move assign an instance of type " + irCtx->color(to_string()) +
		                 " as it does not support simple-move",
		             None);
	}
}

bool Type::is_destructible() const { return has_simple_move(); }

void Type::destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
	if (has_simple_move()) {
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(get_llvm_type()), instance->get_llvm());
	} else {
		irCtx->Error("Could not destroy an instance of type " + irCtx->color(to_string()) +
		                 " as it does not support simple-move",
		             None);
	}
}

bool Type::is_type_definition() const {
	return (type_kind() == TypeKind::DEFINITION) || ((type_kind() == TypeKind::OPAQUE) && as_opaque()->has_subtype() &&
	                                                 as_opaque()->get_subtype()->is_type_definition());
}

DefinitionType* Type::as_type_definition() const { return (DefinitionType*)this; }

bool Type::is_integer() const {
	return (type_kind() == TypeKind::INTEGER) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_integer()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_integer());
}

IntegerType* Type::as_integer() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_integer()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_integer() : (IntegerType*)this);
}

bool Type::is_unsigned() const {
	return ((type_kind() == TypeKind::UNSIGNED_INTEGER) && not((ir::UnsignedType*)this)->is_this_bool_type()) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_unsigned()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_unsigned());
}

UnsignedType* Type::as_unsigned() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_unsigned()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_unsigned() : (UnsignedType*)this);
}

bool Type::is_underlying_type_integer() const {
	return is_integer() || (is_native_type() && as_native_type()->get_subtype()->is_integer());
}

IntegerType* Type::get_underlying_integer_type() const {
	if (is_integer()) {
		return as_integer();
	} else {
		return as_native_type()->get_subtype()->as_integer();
	}
}

bool Type::is_underlying_type_unsigned() const {
	return is_unsigned() || (is_native_type() && as_native_type()->get_subtype()->is_unsigned());
}

UnsignedType* Type::get_underlying_unsigned_type() const {
	if (is_unsigned()) {
		return as_unsigned();
	} else {
		return as_native_type()->get_subtype()->as_unsigned();
	}
}

bool Type::is_bool() const { return (type_kind() == TypeKind::UNSIGNED_INTEGER) && as_unsigned()->is_this_bool_type(); }

UnsignedType* Type::as_bool() const { return as_unsigned(); }

bool Type::is_float() const {
	return (type_kind() == TypeKind::FLOAT) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_float()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_float());
}

FloatType* Type::as_float() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_float()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_float() : (FloatType*)this);
}

bool Type::is_ref() const {
	return (type_kind() == TypeKind::REFERENCE) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_ref()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_ref());
}

RefType* Type::as_ref() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_ref()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_ref() : (RefType*)this);
}

bool Type::is_poly() const {
	return (type_kind() == TypeKind::POLYMORPH) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_poly()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_poly());
}

Polymorph* Type::as_poly() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_poly()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_poly() : (Polymorph*)this);
}

bool Type::is_mark() const {
	return (type_kind() == TypeKind::MARK) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_mark()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_mark());
}

MarkType* Type::as_mark() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_mark()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_mark() : (MarkType*)this);
}

bool Type::is_array() const {
	return (type_kind() == TypeKind::ARRAY) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_array()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_array());
}

ArrayType* Type::as_array() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_array()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_array() : (ArrayType*)this);
}

bool Type::is_vector() const {
	return (type_kind() == TypeKind::VECTOR) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_vector()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_vector());
}

VectorType* Type::as_vector() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_vector()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_vector() : (VectorType*)this);
}

bool Type::is_tuple() const {
	return (type_kind() == TypeKind::TUPLE) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_tuple()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_tuple());
}

TupleType* Type::as_tuple() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_tuple()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_tuple() : (TupleType*)this);
}

bool Type::is_function() const {
	return (type_kind() == TypeKind::FUNCTION) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_function()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_function());
}

FunctionType* Type::as_function() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_function()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_function() : (FunctionType*)this);
}

bool Type::is_struct() const {
	return (type_kind() == TypeKind::STRUCT) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_struct()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_struct());
}

StructType* Type::as_struct() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_struct()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_struct() : (StructType*)this);
}

bool Type::is_mix() const {
	return (type_kind() == TypeKind::MIX) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_mix()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_mix());
}

MixType* Type::as_mix() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_mix()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_mix() : (MixType*)this);
}

bool Type::is_choice() const {
	return ((type_kind() == TypeKind::CHOICE) ||
	        (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_choice()) ||
	        (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_choice()));
}

ChoiceType* Type::as_choice() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_choice()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_choice() : (ChoiceType*)this);
}

bool Type::is_void() const {
	return (type_kind() == TypeKind::VOID) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_void()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_void());
}

bool Type::is_text() const {
	return (type_kind() == TypeKind::TEXT) ||
	       (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_text()) ||
	       (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_text());
}

TextType* Type::as_text() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_text()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_text() : (TextType*)this);
}

bool Type::is_native_type() const {
	return ((type_kind() == TypeKind::NATIVE) ||
	        (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_native_type()) ||
	        (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_native_type()));
}

NativeType* Type::as_native_type() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_native_type()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_native_type() : (NativeType*)this);
}

bool Type::is_future() const {
	return ((type_kind() == TypeKind::FUTURE) ||
	        (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_future()) ||
	        (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_future()));
}

FutureType* Type::as_future() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_future()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_future() : (FutureType*)this);
}

bool Type::is_maybe() const {
	return ((type_kind() == TypeKind::MAYBE) ||
	        (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_maybe()) ||
	        (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_maybe()));
}

MaybeType* Type::as_maybe() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_maybe()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_maybe() : (MaybeType*)this);
}

bool Type::is_region() const {
	return ((type_kind() == TypeKind::REGION) ||
	        (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_region()) ||
	        (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_region()));
}

Region* Type::as_region() const {
	return (type_kind() == TypeKind::DEFINITION)
	           ? ((DefinitionType*)this)->get_subtype()->as_region()
	           : (is_opaque() ? as_opaque()->get_subtype()->as_region() : (Region*)this);
}

bool Type::is_result() const {
	return ((type_kind() == TypeKind::RESULT) ||
	        (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_result()) ||
	        (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_result()));
}

ResultType* Type::as_result() const {
	return ((type_kind() == TypeKind::DEFINITION)
	            ? ((DefinitionType*)this)->get_subtype()->as_result()
	            : (is_opaque() ? as_opaque()->get_subtype()->as_result() : (ResultType*)this));
}

bool Type::is_flag() const {
	return ((type_kind() == TypeKind::FLAG) ||
	        (is_opaque() && as_opaque()->has_subtype() && as_opaque()->get_subtype()->is_flag()) ||
	        (type_kind() == TypeKind::DEFINITION && as_type_definition()->get_subtype()->is_flag()));
}

FlagType* Type::as_flag() const {
	return ((type_kind() == TypeKind::DEFINITION)
	            ? ((DefinitionType*)this)->get_subtype()->as_flag()
	            : (is_opaque() ? as_opaque()->get_subtype()->as_flag() : (FlagType*)this));
}

} // namespace qat::ir
