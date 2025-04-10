#ifndef QAT_IR_TYPES_TYPE_HPP
#define QAT_IR_TYPES_TYPE_HPP

#include "../../utils/macros.hpp"
#include "../uniq.hpp"
#include "./type_kind.hpp"

namespace llvm {
class Type;
class LLVMContext;
} // namespace llvm

namespace qat::ir {

class ArrayType;
class ChoiceType;
class NativeType;
class Ctx;
class DoneSkill;
class IntegerType;
class UnsignedType;
class FloatType;
class CharType;
class RefType;
class SliceType;
class PtrType;
class TupleType;
class FunctionType;
class TextType;
class StructType;
class DefinitionType;
class MixType;
class FutureType;
class MaybeType;
class Region;
class ExpandedType;
class Function;
class Value;
class OpaqueType;
class TypedType;
class PrerunValue;
class ResultType;
class VectorType;
class Polymorph;
class FlagType;
struct TypeInfo;

// Type is the base class for all types in the IR
class Type : public Uniq {
	friend DoneSkill;
	friend TypeInfo;

  protected:
	static Vec<Type*> allTypes;

	String          linkingName;
	llvm::Type*     llvmType;
	Vec<DoneSkill*> defaultImplementations;
	Vec<DoneSkill*> doneSkills;

	TypeInfo* typeInfo = nullptr;

  public:
	Type();
	virtual ~Type();
	static void clear_all();

	useit bool has_default_implementations() const { return not defaultImplementations.empty(); }

	useit Vec<DoneSkill*> const& get_default_implementations() const { return defaultImplementations; }

	useit virtual bool          can_be_prerun_generic() const;
	useit virtual Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const;
	useit virtual bool          is_type_sized() const;
	useit virtual Maybe<bool>   equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const;
	useit String                get_name_for_linking() const;

	useit static Vec<Region*> allRegions();
	useit bool                is_same(Type* other);
	useit bool                isCompatible(Type* other);

	useit virtual bool  is_expanded() const;
	useit ExpandedType* as_expanded() const;

	useit virtual bool can_be_prerun() const;
	useit virtual bool has_prerun_default_value() const;
	useit virtual bool is_default_constructible() const;
	useit virtual bool is_copy_constructible() const;
	useit virtual bool is_copy_assignable() const;
	useit virtual bool is_move_constructible() const;
	useit virtual bool is_move_assignable() const;
	useit virtual bool is_destructible() const;
	useit virtual bool has_simple_copy() const;
	useit virtual bool has_simple_move() const;

	virtual ir::PrerunValue* get_prerun_default_value(ir::Ctx* irCtx);

	virtual void default_construct_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun);
	virtual void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun);
	virtual void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun);
	virtual void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun);
	virtual void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun);
	virtual void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun);

	useit bool        is_opaque() const;
	useit OpaqueType* as_opaque() const;

	useit bool            is_type_definition() const;
	useit DefinitionType* as_type_definition() const;

	useit bool         is_integer() const;
	useit IntegerType* as_integer() const;

	useit bool          is_unsigned() const;
	useit UnsignedType* as_unsigned() const;

	useit bool          is_underlying_type_integer() const;
	useit bool          is_underlying_type_unsigned() const;
	useit IntegerType*  get_underlying_integer_type() const;
	useit UnsignedType* get_underlying_unsigned_type() const;

	useit bool          is_bool() const;
	useit UnsignedType* as_bool() const;

	useit bool       is_float() const;
	useit FloatType* as_float() const;

	useit bool      is_char() const;
	useit CharType* as_char() const;

	useit bool     is_ref() const;
	useit RefType* as_ref() const;

	useit bool       is_poly() const;
	useit Polymorph* as_poly() const;

	useit bool     is_ptr() const;
	useit PtrType* as_ptr() const;

	useit bool       is_slice() const;
	useit SliceType* as_slice() const;

	useit bool       is_array() const;
	useit ArrayType* as_array() const;

	useit bool       is_tuple() const;
	useit TupleType* as_tuple() const;

	useit bool          is_function() const;
	useit FunctionType* as_function() const;

	useit bool        is_struct() const;
	useit StructType* as_struct() const;

	useit bool     is_mix() const;
	useit MixType* as_mix() const;

	useit bool        is_choice() const;
	useit ChoiceType* as_choice() const;

	useit bool      is_text() const;
	useit TextType* as_text() const;

	useit bool        is_future() const;
	useit FutureType* as_future() const;

	useit bool       is_maybe() const;
	useit MaybeType* as_maybe() const;

	useit bool    is_region() const;
	useit Region* as_region() const;

	useit bool is_void() const;

	useit bool       is_typed() const;
	useit TypedType* as_typed() const;

	useit bool        is_native_type() const;
	useit NativeType* as_native_type() const;

	useit bool        is_result() const;
	useit ResultType* as_result() const;

	useit bool        is_vector() const;
	useit VectorType* as_vector() const;

	useit bool      is_flag() const;
	useit FlagType* as_flag() const;

	useit virtual TypeKind type_kind() const = 0;
	useit virtual String   to_string() const = 0;

	useit llvm::Type* get_llvm_type() const { return llvmType; }
};

} // namespace qat::ir

#endif
