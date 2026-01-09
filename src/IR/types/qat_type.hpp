#ifndef QAT_IR_TYPES_TYPE_HPP
#define QAT_IR_TYPES_TYPE_HPP

#include "../../utils/macros.hpp"
#include "../uniq.hpp"
#include "./type_kind.hpp"

#include <unordered_map>

namespace llvm {
class Type;
class LLVMContext;
} // namespace llvm

namespace qat::ir {

class AtomicType;
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
class ToggleType;
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
class ErrorType;
class VectorType;
class Polymorph;
class FlagType;
struct TypeInfo;
class Skill;
class Method;

// Type is the base class for all types in the IR
class Type : public Uniq {
	friend DoneSkill;
	friend TypeInfo;
	friend Method;

  protected:
	static Vec<Type*> allTypes;

	String      linkingName;
	llvm::Type* llvmType;

	Vec<DoneSkill*>                             defaultImplementations;
	std::unordered_map<Skill*, DoneSkill*>      unnamedImplementations;
	std::unordered_multimap<Skill*, DoneSkill*> namedImplementations;
	std::unordered_multimap<String, Skill*>     methodToSkillsMapping;

	TypeInfo* typeInfo = nullptr;

  public:
	Type();
	virtual ~Type() = default;

	static void clear_all();

	bool has_default_implementations() const { return not defaultImplementations.empty(); }

	Vec<DoneSkill*> const& get_default_implementations() const { return defaultImplementations; }

	bool has_unnamed_implementation_for(Skill* skill) const { return unnamedImplementations.contains(skill); }

	DoneSkill* get_unnamed_implementation_for(Skill* skill) const { return unnamedImplementations.at(skill); }

	bool has_named_implementation_for(Skill* skill) const { return namedImplementations.contains(skill); }

	auto get_named_implementations_for(Skill* skill) const { return namedImplementations.equal_range(skill); }

	bool has_skills_for_method_name(String const& mName) const { return methodToSkillsMapping.contains(mName); }

	auto get_skills_for_method_name(String const& mName) const { return methodToSkillsMapping.equal_range(mName); }

	virtual bool          can_be_prerun_generic() const;
	virtual Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const;
	virtual bool          is_type_sized() const;
	virtual Maybe<bool>   equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const;
	String                get_name_for_linking() const;

	static Vec<Region*> allRegions();

	bool is_same(Type const* other) const;
	bool is_compatible_with(Type const* candidate) const;

	virtual bool  is_expanded() const;
	ExpandedType* as_expanded() const;

	virtual bool can_be_prerun() const;
	virtual bool has_prerun_default_value() const;
	virtual bool is_default_constructible() const;
	virtual bool is_copy_constructible() const;
	virtual bool is_copy_assignable() const;
	virtual bool is_move_constructible() const;
	virtual bool is_move_assignable() const;
	virtual bool is_destructible() const;
	virtual bool has_simple_copy() const;
	virtual bool has_simple_move() const;

	virtual ir::PrerunValue* get_prerun_default_value(ir::Ctx* irCtx);

	virtual void default_construct_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun);
	virtual void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun);
	virtual void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun);
	virtual void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun);
	virtual void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun);
	virtual void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun);

	bool        is_opaque() const;
	OpaqueType* as_opaque() const;

	bool            is_type_definition() const;
	DefinitionType* as_type_definition() const;

	bool        is_atomic() const;
	AtomicType* as_atomic() const;

	bool         is_integer() const;
	IntegerType* as_integer() const;

	bool          is_unsigned() const;
	UnsignedType* as_unsigned() const;

	bool is_underlying_type_integer() const;
	bool is_underlying_type_unsigned() const;
	bool is_underlying_type_float() const;

	IntegerType*  get_underlying_integer_type() const;
	UnsignedType* get_underlying_unsigned_type() const;

	bool          is_bool() const;
	UnsignedType* as_bool() const;

	bool       is_float() const;
	FloatType* as_float() const;

	bool      is_char() const;
	CharType* as_char() const;

	bool     is_ref() const;
	RefType* as_ref() const;

	bool       is_poly() const;
	Polymorph* as_poly() const;

	bool     is_ptr() const;
	PtrType* as_ptr() const;

	bool       is_slice() const;
	SliceType* as_slice() const;

	bool       is_array() const;
	ArrayType* as_array() const;

	bool       is_tuple() const;
	TupleType* as_tuple() const;

	bool          is_function() const;
	FunctionType* as_function() const;

	bool        is_struct() const;
	StructType* as_struct() const;

	bool     is_mix() const;
	MixType* as_mix() const;

	bool        is_toggle() const;
	ToggleType* as_toggle() const;

	bool        is_choice() const;
	ChoiceType* as_choice() const;

	bool      is_text() const;
	TextType* as_text() const;

	bool        is_future() const;
	FutureType* as_future() const;

	bool       is_maybe() const;
	MaybeType* as_maybe() const;

	bool    is_region() const;
	Region* as_region() const;

	bool is_void() const;

	bool       is_typed() const;
	TypedType* as_typed() const;

	bool        is_native_type() const;
	NativeType* as_native_type() const;

	bool        is_result() const;
	ResultType* as_result() const;

	bool       is_error() const;
	ErrorType* as_error() const;

	bool        is_vector() const;
	VectorType* as_vector() const;

	bool      is_flag() const;
	FlagType* as_flag() const;

	virtual TypeKind type_kind() const = 0;
	virtual String   to_string() const = 0;

	llvm::Type* get_llvm_type() const { return llvmType; }
};

} // namespace qat::ir

#endif
