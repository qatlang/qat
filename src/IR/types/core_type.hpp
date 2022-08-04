#ifndef QAT_IR_TYPES_CORE_TYPE_HPP
#define QAT_IR_TYPES_CORE_TYPE_HPP

#include "../../utils/visibility.hpp"
#include "../definable.hpp"
#include "../member_function.hpp"
#include "../static_member.hpp"
#include "./qat_type.hpp"
#include <string>
#include <utility>
#include <vector>

namespace qat::IR {

/**
 *  This represents a core type in the language
 *
 */
class CoreType : public QatType, public Definable, public Uniq {
public:
  /**
   *  Member represents an individual member of a core
   * type
   *
   */
  class Member {
  public:
    Member(String _name, QatType *_type, bool _variability,
           const utils::VisibilityInfo &_visibility)
        : name(std::move(_name)), type(_type), visibility(_visibility),
          variability(_variability) {}

    /**
     *  Name of the member
     *
     */
    String name;

    /**
     *  LLVM Type of the member
     *
     */
    QatType *type;

    /**
     *  VisibilityInfo of the member
     *
     */
    utils::VisibilityInfo visibility;

    /**
     *  Variability of the field. The default behaviour is true
     *
     */
    bool variability;
  };

private:
  // Name of the core type
  String name;

  // Parent module
  QatModule *parent;

  // Member fields
  Vec<Member *> members;

  // Static member fields
  Vec<StaticMember *> staticMembers;

  // All member functions (non-static) of this type
  Vec<MemberFunction *> memberFunctions;

  // All static member functions of this type
  Vec<MemberFunction *> staticFunctions;

  // The destructor function of the core type
  Maybe<MemberFunction *> destructor;

  // VisibilityInfo of this CoreType
  utils::VisibilityInfo visibility;

  // TODO - Add support for extension functions

public:
  CoreType(QatModule *mod, String _name, Vec<Member *> _members,
           Vec<MemberFunction *>        _memberFunctions,
           const utils::VisibilityInfo &_visibility);

  /**
   *  Get the full name of this type
   *
   * @return String
   */
  useit String getFullName() const;

  /**
   *  Get the name of this type only, without the prefix of the parent scopes
   */
  useit String getName() const;

  /**
   *  Get the index of the member field with the provided name
   *
   * @param member Name of the member to find
   * @return int Index of the corresponding field
   */
  useit Maybe<usize> getIndexOf(const String &member) const;

  useit u64 getMemberCount() const;

  Member *getMemberAt(u64 index);

  /**
   *  Get the name of the member field at the provided index
   *
   * @param index Index of the member field
   * @return String Name of the member corresponding to that field
   */
  useit String getMemberNameAt(u64 index) const;

  useit QatType *getTypeOfMember(const String &member) const;

  /**
   *  Whether there is a static member in this type with the provided
   * name
   *
   * @param name Name to check for
   * @return true If there is a static member
   * @return false If there is no static member
   */
  useit bool hasStatic(const String &name) const;

  /**
   *  Whether there is a non-static member function of this type with the
   * provided name
   *
   * @param fnName Name of the function
   * @return true
   * @return false
   */
  useit bool hasMemberFunction(const String &fnName) const;

  /**
   *  Get the member function of this type with the provided name
   *
   * @param fnName Name of the function
   * @return const IR::QatMemberFunction &
   */
  useit const MemberFunction *getMemberFunction(const String &fnName) const;

  /**
   *  Whether there is a static member function of this type with the
   * provided name
   *
   * @param fnName Name of the function
   * @return true
   * @return false
   */
  useit bool hasStaticFunction(const String &fnName) const;

  /**
   *  Get the static member function of this type with the provided name
   *
   * @param fnName Name of the function
   * @return const QatMemberFunction&
   */
  useit const MemberFunction *getStaticFunction(const String &fnName) const;

  /**
   *  Add a member function (non-static) to this core type
   *
   * @param mod LLVM Module to insert the function to
   * @param name Name of the function
   * @param is_variation Whether this function is a variation for this core
   * type
   * @param return_type The type of the return value of the function
   * @param args The arguments of the function
   * @param has_variadic_args Whether this function has variable number of
   * arguments
   * @param returns_reference Whether this function returns reference to another
   * value
   * @param visib_info VisibilityInfo of the function
   */
  void addMemberFunction(const String &name, bool is_variation,
                         QatType *return_type, bool is_return_type_variable,
                         bool is_async, Vec<Argument> args,
                         bool                         has_variadic_args,
                         const utils::FileRange      &fileRange,
                         const utils::VisibilityInfo &visibilityInfo);

  // NOTE - Add documentation
  void addStaticFunction(const String &name, QatType *return_type,
                         bool is_return_type_variable, bool is_async,
                         Vec<Argument> args, bool has_variadic_args,
                         const utils::FileRange      &fileRange,
                         const utils::VisibilityInfo &visibilityInfo);

  void addStaticMember(const String &name, QatType *type, bool variability,
                       Value *initial, const utils::VisibilityInfo &visibility);

  useit utils::VisibilityInfo getVisibility() const;

  QatModule *getParent();

  useit TypeKind typeKind() const override;

  useit String toString() const override;

  void defineLLVM(llvmHelper &helper) const override;

  void defineCPP(cpp::File &file) const override;

  useit llvm::Type *emitLLVM(llvmHelper &help) const override;

  void emitCPP(cpp::File &file) const override;

  useit nuo::Json toJson() const override;
};

} // namespace qat::IR

#endif