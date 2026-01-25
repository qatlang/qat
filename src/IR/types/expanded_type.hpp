#ifndef QAT_IR_TYPES_EXPANDED_TYPE_HPP
#define QAT_IR_TYPES_EXPANDED_TYPE_HPP

#include "../generics.hpp"
#include "../method.hpp"
#include "./opaque.hpp"
#include "./qat_type.hpp"

namespace qat::ast {
class DefineStructType;
class DefineMixType;
} // namespace qat::ast

namespace qat::ir {

class ExpandedType : public Type, public Mentionable {
	friend class Method;
	friend class DefinitionType;
	friend class ast::DefineStructType;
	friend class ast::DefineMixType;
	friend class ast::ConvertorPrototype;

  protected:
	Identifier            name;
	Vec<GenericArgument*> generics;
	Mod*                  parent = nullptr;

	Vec<DefinitionType*> definitions;

	Method*      defaultConstructor = nullptr;
	Vec<Method*> memberFunctions;          // Normal
	Vec<Method*> valuedMemberFunctions;    // Valued parent
	Vec<Method*> normalBinaryOperators;    // Normal
	Vec<Method*> variationBinaryOperators; // Variation

	Vec<Method*> unaryOperators;            //
	Vec<Method*> constructors;              // Constructors
	Vec<Method*> fromConvertors;            // From Convertors
	Vec<Method*> toConvertors;              // To Convertors
	Vec<Method*> staticFunctions;           // Static
	Method*      copyConstructor = nullptr; // Copy constructor
	Method*      moveConstructor = nullptr; // Move constructor
	Method*      copyAssignment  = nullptr; // Copy assignment operator
	Method*      moveAssignment  = nullptr; // Move assignment operator

	bool hasSimpleCopy       = true;
	bool hasSimpleMove       = true;
	bool isCopyConstructible = false;
	bool isMoveConstructible = false;
	bool isCopyAssignable    = false;
	bool isMoveAssignable    = false;
	bool isDestructible      = false;

	bool needsImplicitDestructor = false;
	bool hasDefinedDestructor    = false;

	Maybe<Method*> destructor; // Destructor

	OpaqueType* opaqueEquivalent = nullptr;

	VisibilityInfo visibility;

	ExpandedType(Identifier _name, Vec<GenericArgument*> _generics, Mod* _parent, const VisibilityInfo& _visib);

  public:
	bool has_simple_copy() const final {
		if (has_copy_constructor() or has_copy_assignment()) {
			return false;
		} else {
			return hasSimpleCopy;
		}
	}

	bool has_simple_move() const final {
		if (has_move_constructor() or has_move_assignment()) {
			return false;
		} else {
			return hasSimpleMove;
		}
	}

	bool is_copy_constructible() const final { return (copyConstructor != nullptr) or isCopyConstructible; }

	bool is_copy_assignable() const final { return (copyAssignment != nullptr) or isCopyAssignable; }

	bool is_move_constructible() const final { return (moveConstructor != nullptr) or isMoveConstructible; }

	bool is_move_assignable() const final { return (moveAssignment != nullptr) or isMoveAssignable; }

	bool is_destructible() const final { return destructor.has_value() or isDestructible; }

	bool is_generic() const;

	bool has_generic_parameter(const String& name) const;

	GenericArgument* get_generic_parameter(const String& name) const;

	bool has_definition(String const& name) const;

	DefinitionType* get_definition(String const& name) const;

	String get_full_name() const;

	Identifier get_name() const;

	static Maybe<Method*> check_variation(Vec<Method*> const& variationFunctions, String const& name);

	bool has_variation(String const& name) const;

	Method* get_variation(const String& name) const;

	static Maybe<Method*> check_normal_method(Vec<Method*> const& memberFunctions, String const& name);

	bool has_normal_method(const String& fnName) const;

	Method* get_normal_method(const String& fnName) const;

	static Maybe<Method*> check_valued_function(Vec<Method*> const& memberFunctions, String const& name);

	bool has_valued_method(String const& name) const;

	Method* get_valued_method(String const& name) const;

	static Maybe<ir::Method*> check_static_method(Vec<Method*> const& staticFns, String const& name);

	bool has_static_method(const String& fnName) const;

	Method* get_static_method(const String& fnName) const;

	static Maybe<ir::Method*> check_binary_operator(Vec<Method*> const& binOps, const String& opr,
	                                                Pair<Maybe<bool>, ir::Type*> argType);

	bool has_normal_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const;

	Method* get_normal_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const;

	bool has_variation_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const;

	Method* get_variation_binary_operator(const String& opr, Pair<Maybe<bool>, ir::Type*> argType) const;

	static Maybe<ir::Method*> check_unary_operator(Vec<Method*> const& unaryOps, String const& opr);

	bool has_unary_operator(const String& opr) const;

	Method* get_unary_operator(const String& opr) const;

	static Maybe<ir::Method*> check_from_convertor(Vec<Method*> const& fromConvs, Maybe<bool> isValueVar,
	                                               ir::Type* argType);

	bool has_from_convertor(Maybe<bool> isValueVar, ir::Type* argType) const;

	Method* get_from_convertor(Maybe<bool> isValueVar, ir::Type* argType) const;

	static Maybe<ir::Method*> check_to_convertor(Vec<Method*> const& toConvertors, ir::Type* targetTy);

	bool has_to_convertor(ir::Type* type) const;

	Method* get_to_convertor(ir::Type* type) const;

	static Maybe<ir::Method*> check_constructor_with_types(Vec<Method*> const&                      cons,
	                                                       Vec<Pair<Maybe<bool>, ir::Type*>> const& types);

	bool has_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> const& types) const;

	Method* get_constructor_with_types(Vec<Pair<Maybe<bool>, ir::Type*>> const& types) const;

	bool has_default_constructor() const;

	Method* get_default_constructor() const;

	bool has_any_from_convertor() const;

	bool has_any_constructor() const;

	bool has_copy_constructor() const;

	Method* get_copy_constructor() const;

	bool has_move_constructor() const;

	Method* get_move_constructor() const;

	bool has_copy_assignment() const;

	Method* get_copy_assignment() const;

	bool has_move_assignment() const;

	Method* get_move_assignment() const;

	bool has_destructor() const;

	Method* get_destructor() const;

	Mod* get_module();

	virtual LinkNames get_link_names() const = 0;

	bool is_accessible(const AccessInfo& reqInfo) const;

	VisibilityInfo get_visibility() const;

	bool is_expanded() const override;
};

} // namespace qat::ir

#endif
