#ifndef QAT_IR_TYPES_EXPANDED_TYPE_HPP
#define QAT_IR_TYPES_EXPANDED_TYPE_HPP

#include "../member_function.hpp"
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

  bool            needsImplicitDestructor = false;
  bool            hasDefinedDestructor    = false;
  MemberFunction* destructor              = nullptr; // Destructor

  bool explicitCopy = false;
  bool explicitMove = false;

  utils::VisibilityInfo visibility;

  ExpandedType(Identifier _name, QatModule* _parent, const utils::VisibilityInfo& _visib);

public:
  useit String          getFullName() const;
  useit Identifier      getName() const;
  useit bool            hasMemberFunction(const String& fnName) const;
  useit MemberFunction* getMemberFunction(const String& fnName) const;
  useit bool            hasStaticFunction(const String& fnName) const;
  useit MemberFunction* getStaticFunction(const String& fnName) const;
  useit bool            hasBinaryOperator(const String& opr, IR::QatType* type) const;
  useit MemberFunction* getBinaryOperator(const String& opr, IR::QatType* type) const;
  useit bool            hasUnaryOperator(const String& opr) const;
  useit MemberFunction* getUnaryOperator(const String& opr) const;
  useit u64             getOperatorVariantIndex(const String& opr) const;
  useit bool            hasFromConvertor(IR::QatType* type) const;
  useit MemberFunction* getFromConvertor(IR::QatType* type) const;
  useit bool            hasToConvertor(IR::QatType* type) const;
  useit MemberFunction* getToConvertor(IR::QatType* type) const;
  useit bool            hasConstructorWithTypes(Vec<IR::QatType*> types) const;
  useit MemberFunction* getConstructorWithTypes(Vec<IR::QatType*> types) const;
  useit bool            hasDefaultConstructor() const;
  useit MemberFunction* getDefaultConstructor() const;
  useit bool            hasAnyFromConvertor() const;
  useit bool            hasAnyConstructor() const;
  useit bool            hasCopyConstructor() const;
  useit MemberFunction* getCopyConstructor() const;
  useit bool            hasMoveConstructor() const;
  useit MemberFunction* getMoveConstructor() const;
  useit bool            hasCopyAssignment() const;
  useit MemberFunction* getCopyAssignment() const;
  useit bool            hasMoveAssignment() const;
  useit MemberFunction* getMoveAssignment() const;
  useit bool            hasCopy() const;
  useit bool            hasMove() const;
  useit bool            hasDestructor() const;
  void                  createDestructor(FileRange range, llvm::LLVMContext& ctx);
  useit MemberFunction* getDestructor() const;
  useit QatModule*      getParent();

  useit bool isTriviallyCopyable() const;

  useit bool isAccessible(const utils::RequesterInfo& reqInfo) const;
  useit utils::VisibilityInfo getVisibility() const;
  useit bool                  isExpanded() const final;
  useit bool                  isDestructible() const final;
  void                        destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) final;
};

} // namespace qat::IR

#endif