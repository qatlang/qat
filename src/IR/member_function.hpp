#ifndef QAT_IR_MEMBER_FUNCTION_HPP
#define QAT_IR_MEMBER_FUNCTION_HPP

#include "../utils/visibility.hpp"
#include "./argument.hpp"
#include "./function.hpp"
#include "link_names.hpp"
#include "skill.hpp"
#include "types/function.hpp"
#include "types/qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

#include <set>
#include <string>
#include <vector>

namespace qat::IR {

enum class MemberFnType {
  normal,
  value_method,
  staticFn,
  fromConvertor,
  toConvertor,
  constructor,
  copyConstructor,
  moveConstructor,
  copyAssignment,
  moveAssignment,
  destructor,
  binaryOperator,
  unaryOperator,
  defaultConstructor,
};

String memberFnTypeToString(MemberFnType type);

class ExpandedType;

enum class MemberParentType { expandedType, doSkill };
class MemberParent {
  void*            data;
  MemberParentType parentType;

  MemberParent(MemberParentType _parentType, void* data);

public:
  useit static MemberParent* CreateFromExpanded(IR::ExpandedType* expTy);
  useit static MemberParent* CreateFromDoSkill(IR::DoSkill* doneSkill);

  useit bool isExpanded() const;
  useit bool isDoneSkill() const;
  useit IR::QatType* getParentType() const;
  useit IR::ExpandedType* asExpanded() const;
  useit IR::DoSkill* asDoneSkill() const;
  useit FileRange    getTypeRange() const;
};

/**
 *  MemberFunction represents a member function for a core type in
 * the language. It can be static or non-static
 *
 */
class MemberFunction : public Function {
private:
  MemberParent* parent;
  bool          isStatic;
  bool          isVariation;
  MemberFnType  fnType;
  Identifier    selfName;

  std::set<String>              usedMembers;
  std::set<IR::MemberFunction*> memberFunctionCalls;
  Vec<Pair<usize, FileRange>>   initTypeMembers;

  static LinkNames getNameInfoFrom(MemberParent* parent, bool isStatic, Identifier name, bool isVar,
                                   MemberFnType fnType, Vec<Argument> args, QatType* retTy);

  MemberFunction(MemberFnType fnType, bool _isVariation, MemberParent* _parent, const Identifier& _name,
                 ReturnType* returnType, Vec<Argument> _args, bool has_variadic_arguments, bool _is_static,
                 Maybe<FileRange> _fileRange, const VisibilityInfo& _visibility_info, IR::Context* ctx);

public:
  static MemberFunction* Create(MemberParent* parent, bool is_variation, const Identifier& name,
                                ReturnType* return_type, const Vec<Argument>& args, bool has_variadic_args,
                                Maybe<FileRange> fileRange, const VisibilityInfo& visib_info, IR::Context* ctx);

  static MemberFunction* CreateValued(MemberParent* parent, const Identifier& name, QatType* return_type,
                                      const Vec<Argument>& args, bool has_variadic_args, Maybe<FileRange> fileRange,
                                      const VisibilityInfo& visib_info, IR::Context* ctx);

  static MemberFunction* DefaultConstructor(MemberParent* parent, FileRange nameRange, const VisibilityInfo& visibInfo,
                                            Maybe<FileRange> fileRange, IR::Context* ctx);

  static MemberFunction* CopyConstructor(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                         Maybe<FileRange> fileRange, IR::Context* ctx);

  static MemberFunction* MoveConstructor(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                         Maybe<FileRange> fileRange, IR::Context* ctx);

  static MemberFunction* CopyAssignment(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                        Maybe<FileRange> fileRange, IR::Context* ctx);

  static MemberFunction* MoveAssignment(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                        const FileRange& fileRange, IR::Context* ctx);

  static MemberFunction* CreateConstructor(MemberParent* parent, FileRange nameRange, const Vec<Argument>& args,
                                           bool hasVariadicArgs, Maybe<FileRange> fileRange,
                                           const VisibilityInfo& visibInfo, IR::Context* ctx);

  static MemberFunction* CreateFromConvertor(MemberParent* parent, FileRange nameRange, QatType* sourceType,
                                             const Identifier& name, Maybe<FileRange> fileRange,
                                             const VisibilityInfo& visibInfo, IR::Context* ctx);

  static MemberFunction* CreateToConvertor(MemberParent* parent, FileRange nameRange, QatType* destType,
                                           Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo,
                                           IR::Context* ctx);

  static MemberFunction* CreateDestructor(MemberParent* parent, FileRange nameRange, Maybe<FileRange> fileRange,
                                          IR::Context* ctx);

  static MemberFunction* CreateOperator(MemberParent* parent, FileRange nameRange, bool isBinary, bool isVariationFn,
                                        const String& opr, ReturnType* returnType, const Vec<Argument>& args,
                                        Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo, IR::Context* ctx);

  static MemberFunction* CreateStatic(MemberParent* parent, const Identifier& name, QatType* return_type,
                                      const Vec<Argument>& args, bool has_variadic_args, Maybe<FileRange> fileRange,
                                      const VisibilityInfo& visib_info, IR::Context* ctx);

  ~MemberFunction() override;

  useit inline Identifier getName() const final {
    switch (fnType) {
      case MemberFnType::normal:
      case MemberFnType::binaryOperator:
      case MemberFnType::unaryOperator:
      case MemberFnType::staticFn: {
        return selfName;
      }
      default:
        return name;
    }
  }

  useit inline MemberFnType getMemberFnType() const { return fnType; }

  useit inline bool isConstructor() const {
    switch (getMemberFnType()) {
      case MemberFnType::fromConvertor:
      case MemberFnType::constructor:
      case MemberFnType::copyConstructor:
      case MemberFnType::moveConstructor:
      case MemberFnType::defaultConstructor:
        return true;
      default:
        return false;
    }
  }

  useit inline bool isVariationFunction() const { return isStatic ? false : isVariation; }

  useit inline bool isStaticFunction() const { return isStatic; }

  useit String getFullName() const final;

  useit bool isMemberFunction() const final;

  useit inline bool isInSkill() const { return parent->isDoneSkill(); }

  useit inline DoSkill* getParentSkill() const { return parent->asDoneSkill(); }

  useit inline QatType* getParentType() { return parent->getParentType(); }

  useit inline Json toJson() const { return Json()._("parentType", parent->getParentType()->getID()); }

  useit inline Maybe<FileRange> isMemberInitted(usize memInd) const {
    for (auto mem : initTypeMembers) {
      if (mem.first == memInd) {
        return mem.second;
      }
    }
    return None;
  }
  useit inline std::set<String>& getUsedMembers() { return usedMembers; }

  void inline addUsedMember(String memberName) { usedMembers.insert(memberName); }

  void inline addMemberFunctionCall(IR::MemberFunction* other) { memberFunctionCalls.insert(other); }

  void inline addInitMember(Pair<usize, FileRange> memInfo) { initTypeMembers.push_back(memInfo); }

  void updateOverview() final;
};

} // namespace qat::IR

#endif