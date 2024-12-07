#ifndef QAT_IR_TYPES_EXPANDED_TYPE_HPP
#define QAT_IR_TYPES_EXPANDED_TYPE_HPP

#include "../generics.hpp"
#include "../method.hpp"
#include "./opaque.hpp"
#include "./qat_type.hpp"

namespace qat::ast {
class DefineCoreType;
class DefineMixType;
} // namespace qat::ast

namespace qat::ir {

class ExpandedType : public Type {
	friend class Method;
	friend class ast::DefineCoreType;
	friend class ast::DefineMixType;
	friend class ast::ConvertorPrototype;

  protected:
	Identifier			  name;
	Vec<GenericArgument*> generics;
	Mod*				  parent			 = nullptr;
	Method*				  defaultConstructor = nullptr;
	Vec<Method*>		  memberFunctions;			// Normal
	Vec<Method*>		  valuedMemberFunctions;	// Valued parent
	Vec<Method*>		  normalBinaryOperators;	// Normal
	Vec<Method*>		  variationBinaryOperators; // Variation

	Vec<Method*>   unaryOperators;	//
	Vec<Method*>   constructors;	// Constructors
	Vec<Method*>   fromConvertors;	// From Convertors
	Vec<Method*>   toConvertors;	// To Convertors
	Vec<Method*>   staticFunctions; // Static
	Maybe<Method*> copyConstructor; // Copy constructor
	Maybe<Method*> moveConstructor; // Move constructor
	Maybe<Method*> copyAssignment;	// Copy assignment operator
	Maybe<Method*> moveAssignment;	// Move assignment operator

	bool explicitTrivialCopy	 = false;
	bool explicitTrivialMove	 = false;
	bool needsImplicitDestructor = false;
	bool hasDefinedDestructor	 = false;

	Maybe<Method*> destructor; // Destructor

	OpaqueType* opaqueEquivalent = nullptr;

	VisibilityInfo visibility;

	ExpandedType(Identifier _name, Vec<GenericArgument*> _generics, Mod* _parent, const VisibilityInfo& _visib);

  public:
	useit bool			   is_generic() const;
	useit bool			   has_generic_parameter(const String& name) const;
	useit GenericArgument* get_generic_parameter(const String& name) const;

	useit String	 get_full_name() const;
	useit Identifier get_name() const;

	useit static Maybe<Method*> check_variation(Vec<Method*> const& variationFunctions, String const& name);
	useit bool					has_variation(String const& name) const;
	useit Method*				get_variation(const String& name) const;

	useit static Maybe<Method*> check_normal_method(Vec<Method*> const& memberFunctions, String const& name);
	useit bool					has_normal_method(const String& fnName) const;
	useit Method*				get_normal_method(const String& fnName) const;

	useit static Maybe<Method*> check_valued_function(Vec<Method*> const& memberFunctions, String const& name);
	useit bool					has_valued_method(String const& name) const;
	useit Method*				get_valued_method(String const& name) const;

	useit static Maybe<ir::Method*> check_static_method(Vec<Method*> const& staticFns, String const& name);
	useit bool						has_static_method(const String& fnName) const;
	useit Method*					get_static_method(const String& fnName) const;

	useit static Maybe<ir::Method*> check_binary_operator(Vec<Method*> const& binOps, const String& opr,
														  Pair<Maybe<bool>, ir::Type*> argType);
	useit bool	  has_normal_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const;
	useit Method* get_normal_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const;

	useit bool	  has_variation_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const;
	useit Method* get_variation_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const;

	useit static Maybe<ir::Method*> check_unary_operator(Vec<Method*> const& unaryOps, String const& opr);
	useit bool						has_unary_operator(const String& opr) const;
	useit Method*					get_unary_operator(const String& opr) const;

	useit static Maybe<ir::Method*> check_from_convertor(Vec<Method*> const& fromConvs, Maybe<bool> isValueVar,
														 ir::Type* argType);
	useit bool						has_from_convertor(Maybe<bool> isValueVar, ir::Type* argType) const;
	useit Method*					get_from_convertor(Maybe<bool> isValueVar, ir::Type* argType) const;

	useit static Maybe<ir::Method*> check_to_convertor(Vec<Method*> const& toConvertors, ir::Type* targetTy);
	useit bool						has_to_convertor(ir::Type* type) const;
	useit Method*					get_to_convertor(ir::Type* type) const;

	useit static Maybe<ir::Method*> check_constructor_with_types(Vec<Method*> const&			   cons,
																 Vec<Pair<Maybe<bool>, ir::Type*>> types);
	useit bool						has_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> types) const;
	useit Method*					get_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> types) const;

	useit bool				has_default_constructor() const;
	useit Method*			get_default_constructor() const;
	useit bool				has_any_from_convertor() const;
	useit bool				has_any_constructor() const;
	useit bool				has_copy_constructor() const;
	useit Method*			get_copy_constructor() const;
	useit bool				has_move_constructor() const;
	useit Method*			get_move_constructor() const;
	useit bool				has_copy_assignment() const;
	useit Method*			get_copy_assignment() const;
	useit bool				has_move_assignment() const;
	useit Method*			get_move_assignment() const;
	useit bool				has_copy() const;
	useit bool				has_move() const;
	useit bool				has_destructor() const;
	useit Method*			get_destructor() const;
	useit Mod*				get_module();
	useit virtual LinkNames get_link_names() const = 0;

	useit bool			 is_accessible(const AccessInfo& reqInfo) const;
	useit VisibilityInfo get_visibility() const;
	useit bool			 is_expanded() const override;
};

} // namespace qat::ir

#endif
