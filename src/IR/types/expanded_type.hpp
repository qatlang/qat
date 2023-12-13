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
  Vec<MemberFunction*>   memberFunctions; // Normal
  Vec<MemberFunction*>   binaryOperators; //
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

  useit String            getFullName() const;
  useit Identifier        getName() const;
  useit bool              hasNormalMemberFn(const String& fnName) const;
  useit bool              hasVariationFn(String const& name) const;
  useit MemberFunction*   getVariationFn(const String& name) const;
  useit MemberFunction*   getNormalMemberFn(const String& fnName) const;
  useit bool              hasStaticFunction(const String& fnName) const;
  useit MemberFunction*   getStaticFunction(const String& fnName) const;
  useit bool              hasBinaryOperator(const String& opr, IR::QatType* type) const;
  useit MemberFunction*   getBinaryOperator(const String& opr, IR::QatType* type) const;
  useit bool              hasUnaryOperator(const String& opr) const;
  useit MemberFunction*   getUnaryOperator(const String& opr) const;
  useit u64               getOperatorVariantIndex(const String& opr) const;
  useit bool              hasFromConvertor(IR::QatType* type) const;
  useit MemberFunction*   getFromConvertor(IR::QatType* type) const;
  useit bool              hasToConvertor(IR::QatType* type) const;
  useit MemberFunction*   getToConvertor(IR::QatType* type) const;
  useit bool              hasConstructorWithTypes(Vec<IR::QatType*> types) const;
  useit MemberFunction*   getConstructorWithTypes(Vec<IR::QatType*> types) const;
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
  void                    createDestructor(FileRange range, IR::Context* ctx);
  useit MemberFunction*   getDestructor() const;
  useit QatModule*        getParent();
  useit virtual LinkNames getLinkNames() const = 0;

  useit bool           isAccessible(const AccessInfo& reqInfo) const;
  useit VisibilityInfo getVisibility() const;
  useit bool           isExpanded() const override;
};

} // namespace qat::IR

#endif