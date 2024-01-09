#ifndef QAT_IR_TYPES_EXPANDED_TYPE_HPP
#define QAT_IR_TYPES_EXPANDED_TYPE_HPP

#include "../generics.hpp"
#include "../member_function.hpp"
#include "opaque.hpp"
#include "qat_type.hpp"

namespace qat::ast {
class DefineCoreType;
class DefineMixType;
} // namespace qat::ast

namespace qat::IR {

class ExpandedType : public QatType {
  friend class MemberFunction;
  friend class ast::DefineCoreType;
  friend class ast::DefineMixType;

protected:
  Identifier             name;
  Vec<GenericParameter*> generics;
  QatModule*             parent             = nullptr;
  MemberFunction*        defaultConstructor = nullptr;
  Vec<MemberFunction*>   memberFunctions;          // Normal
  Vec<MemberFunction*>   normalBinaryOperators;    // Normal
  Vec<MemberFunction*>   variationBinaryOperators; // Variation

  Vec<MemberFunction*>   unaryOperators;  //
  Vec<MemberFunction*>   constructors;    // Constructors
  Vec<MemberFunction*>   fromConvertors;  // From Convertors
  Vec<MemberFunction*>   toConvertors;    // To Convertors
  Vec<MemberFunction*>   staticFunctions; // Static
  Maybe<MemberFunction*> copyConstructor; // Copy constructor
  Maybe<MemberFunction*> moveConstructor; // Move constructor
  Maybe<MemberFunction*> copyAssignment;  // Copy assignment operator
  Maybe<MemberFunction*> moveAssignment;  // Move assignment operator

  bool explicitTrivialCopy     = false;
  bool explicitTrivialMove     = false;
  bool needsImplicitDestructor = false;
  bool hasDefinedDestructor    = false;

  Maybe<MemberFunction*> destructor; // Destructor

  OpaqueType* opaqueEquivalent = nullptr;

  VisibilityInfo visibility;

  ExpandedType(Identifier _name, Vec<GenericParameter*> _generics, QatModule* _parent, const VisibilityInfo& _visib);

public:
  useit bool              isGeneric() const;
  useit bool              hasGenericParameter(const String& name) const;
  useit GenericParameter* getGenericParameter(const String& name) const;

  useit String     getFullName() const;
  useit Identifier getName() const;

  useit static Maybe<MemberFunction*> checkVariationFn(Vec<MemberFunction*> const& variationFunctions,
                                                       String const&               name);
  useit bool                          hasVariationFn(String const& name) const;
  useit MemberFunction*               getVariationFn(const String& name) const;

  useit static Maybe<MemberFunction*> checkNormalMemberFn(Vec<MemberFunction*> const& memberFunctions,
                                                          String const&               name);
  useit bool                          hasNormalMemberFn(const String& fnName) const;
  useit MemberFunction*               getNormalMemberFn(const String& fnName) const;

  useit static Maybe<IR::MemberFunction*> checkStaticFunction(Vec<MemberFunction*> const& staticFns,
                                                              String const&               name);
  useit bool                              hasStaticFunction(const String& fnName) const;
  useit MemberFunction*                   getStaticFunction(const String& fnName) const;

  useit static Maybe<IR::MemberFunction*> checkBinaryOperator(Vec<MemberFunction*> const& binOps, const String& opr,
                                                              Pair<Maybe<bool>, IR::QatType*> argType);
  useit bool            hasNormalBinaryOperator(const String& opr, Pair<Maybe<bool>, IR::QatType*> argType) const;
  useit MemberFunction* getNormalBinaryOperator(const String& opr, Pair<Maybe<bool>, IR::QatType*> argType) const;

  useit bool            hasVariationBinaryOperator(const String& opr, Pair<Maybe<bool>, IR::QatType*> argType) const;
  useit MemberFunction* getVariationBinaryOperator(const String& opr, Pair<Maybe<bool>, IR::QatType*> argType) const;

  useit static Maybe<IR::MemberFunction*> checkUnaryOperator(Vec<MemberFunction*> const& unaryOps, String const& opr);
  useit bool                              hasUnaryOperator(const String& opr) const;
  useit MemberFunction*                   getUnaryOperator(const String& opr) const;

  useit static Maybe<IR::MemberFunction*> checkFromConvertor(Vec<MemberFunction*> const& fromConvs,
                                                             Maybe<bool> isValueVar, IR::QatType* argType);
  useit bool                              hasFromConvertor(Maybe<bool> isValueVar, IR::QatType* argType) const;
  useit MemberFunction*                   getFromConvertor(Maybe<bool> isValueVar, IR::QatType* argType) const;

  useit static Maybe<IR::MemberFunction*> checkToConvertor(Vec<MemberFunction*> const& toConvertors,
                                                           IR::QatType*                targetTy);
  useit bool                              hasToConvertor(IR::QatType* type) const;
  useit MemberFunction*                   getToConvertor(IR::QatType* type) const;

  useit static Maybe<IR::MemberFunction*> checkConstructorWithTypes(Vec<MemberFunction*> const&          cons,
                                                                    Vec<Pair<Maybe<bool>, IR::QatType*>> types);
  useit bool                              hasConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> types) const;
  useit MemberFunction*                   getConstructorWithTypes(Vec<Pair<Maybe<bool>, IR::QatType*>> types) const;

  useit bool              hasDefaultConstructor() const;
  useit MemberFunction*   getDefaultConstructor() const;
  useit bool              hasAnyFromConvertor() const;
  useit bool              hasAnyConstructor() const;
  useit bool              hasCopyConstructor() const;
  useit MemberFunction*   getCopyConstructor() const;
  useit bool              hasMoveConstructor() const;
  useit MemberFunction*   getMoveConstructor() const;
  useit bool              hasCopyAssignment() const;
  useit MemberFunction*   getCopyAssignment() const;
  useit bool              hasMoveAssignment() const;
  useit MemberFunction*   getMoveAssignment() const;
  useit bool              hasCopy() const;
  useit bool              hasMove() const;
  useit bool              hasDestructor() const;
  useit MemberFunction*   getDestructor() const;
  useit QatModule*        getParent();
  useit virtual LinkNames getLinkNames() const = 0;

  useit bool           isAccessible(const AccessInfo& reqInfo) const;
  useit VisibilityInfo getVisibility() const;
  useit bool           isExpanded() const override;
};

} // namespace qat::IR

#endif