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

namespace qat::ir {

enum class MethodType {
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

String memberFnTypeToString(MethodType type);

class ExpandedType;

enum class MemberParentType { expandedType, doSkill };
class MemberParent {
  static Vec<MemberParent*> allMemberParents;

  void*            data;
  MemberParentType parentType;

public:
  MemberParent(MemberParentType _parentType, void* data);
  useit static MemberParent* create_expanded_type(ir::ExpandedType* expTy);
  useit static MemberParent* create_do_skill(ir::DoneSkill* doneSkill);

  useit bool is_expanded() const;
  useit bool is_done_skill() const;
  useit ir::Type* get_parent_type() const;
  useit ir::ExpandedType* as_expanded() const;
  useit ir::DoneSkill* as_done_skill() const;
  useit FileRange      get_type_range() const;
  useit ir::Mod* get_module() const;

  useit bool is_same(ir::MemberParent* other);
};

class Method : public Function {
private:
  MemberParent* parent;
  bool          isStatic;
  bool          isVariation;
  MethodType    fnType;
  Identifier    selfName;

  std::set<String>            usedMembers;
  std::set<ir::Method*>       memberFunctionCalls;
  Vec<Pair<usize, FileRange>> initTypeMembers;

  static LinkNames get_link_names_from(MemberParent* parent, bool isStatic, Identifier name, bool isVar,
                                       MethodType fnType, Vec<Argument> args, Type* retTy);

  Method(MethodType fnType, bool _isVariation, MemberParent* _parent, const Identifier& _name, ReturnType* returnType,
         Vec<Argument> _args, bool has_variadic_arguments, bool _is_static, Maybe<FileRange> _fileRange,
         const VisibilityInfo& _visibility_info, ir::Ctx* irCtx);

public:
  static Method* Create(MemberParent* parent, bool is_variation, const Identifier& name, ReturnType* return_type,
                        const Vec<Argument>& args, bool has_variadic_args, Maybe<FileRange> fileRange,
                        const VisibilityInfo& visib_info, ir::Ctx* irCtx);

  static Method* CreateValued(MemberParent* parent, const Identifier& name, Type* return_type,
                              const Vec<Argument>& args, bool has_variadic_args, Maybe<FileRange> fileRange,
                              const VisibilityInfo& visib_info, ir::Ctx* irCtx);

  static Method* DefaultConstructor(MemberParent* parent, FileRange nameRange, const VisibilityInfo& visibInfo,
                                    Maybe<FileRange> fileRange, ir::Ctx* irCtx);

  static Method* CopyConstructor(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                 Maybe<FileRange> fileRange, ir::Ctx* irCtx);

  static Method* MoveConstructor(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                 Maybe<FileRange> fileRange, ir::Ctx* irCtx);

  static Method* CopyAssignment(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                Maybe<FileRange> fileRange, ir::Ctx* irCtx);

  static Method* MoveAssignment(MemberParent* parent, FileRange nameRange, const Identifier& otherName,
                                const FileRange& fileRange, ir::Ctx* irCtx);

  static Method* CreateConstructor(MemberParent* parent, FileRange nameRange, const Vec<Argument>& args,
                                   bool hasVariadicArgs, Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo,
                                   ir::Ctx* irCtx);

  static Method* CreateFromConvertor(MemberParent* parent, FileRange nameRange, Type* sourceType,
                                     const Identifier& name, Maybe<FileRange> fileRange,
                                     const VisibilityInfo& visibInfo, ir::Ctx* irCtx);

  static Method* CreateToConvertor(MemberParent* parent, FileRange nameRange, Type* destType,
                                   Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo, ir::Ctx* irCtx);

  static Method* CreateDestructor(MemberParent* parent, FileRange nameRange, Maybe<FileRange> fileRange,
                                  ir::Ctx* irCtx);

  static Method* CreateOperator(MemberParent* parent, FileRange nameRange, bool isBinary, bool isVariationFn,
                                const String& opr, ReturnType* returnType, const Vec<Argument>& args,
                                Maybe<FileRange> fileRange, const VisibilityInfo& visibInfo, ir::Ctx* irCtx);

  static Method* CreateStatic(MemberParent* parent, const Identifier& name, Type* return_type,
                              const Vec<Argument>& args, bool has_variadic_args, Maybe<FileRange> fileRange,
                              const VisibilityInfo& visib_info, ir::Ctx* irCtx);

  ~Method() override;

  useit inline Identifier get_name() const final {
    switch (fnType) {
      case MethodType::normal:
      case MethodType::binaryOperator:
      case MethodType::unaryOperator:
      case MethodType::staticFn: {
        return selfName;
      }
      default:
        return name;
    }
  }

  useit inline MethodType get_method_type() const { return fnType; }

  useit inline bool isConstructor() const {
    switch (get_method_type()) {
      case MethodType::fromConvertor:
      case MethodType::constructor:
      case MethodType::copyConstructor:
      case MethodType::moveConstructor:
      case MethodType::defaultConstructor:
        return true;
      default:
        return false;
    }
  }

  useit inline bool isVariationFunction() const { return isStatic ? false : isVariation; }

  useit inline bool isStaticFunction() const { return isStatic; }

  useit String get_full_name() const final;

  useit bool isMemberFunction() const final;

  useit inline bool isInSkill() const { return parent->is_done_skill(); }

  useit inline DoneSkill* getParentSkill() const { return parent->as_done_skill(); }

  useit inline Type* get_parent_type() { return parent->get_parent_type(); }

  useit inline Json to_json() const { return Json()._("parentType", parent->get_parent_type()->get_id()); }

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

  void inline addMemberFunctionCall(ir::Method* other) { memberFunctionCalls.insert(other); }

  void inline addInitMember(Pair<usize, FileRange> memInfo) { initTypeMembers.push_back(memInfo); }

  void update_overview() final;
};

} // namespace qat::ir

#endif