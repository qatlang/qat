#ifndef QAT_IR_TYPES_CORE_TYPE_HPP
#define QAT_IR_TYPES_CORE_TYPE_HPP

#include "../../utils/visibility.hpp"
#include "../definable.hpp"
#include "../member_function.hpp"
#include "../static_member.hpp"
#include "./qat_type.hpp"
#include <string>
#include <vector>

namespace qat::IR {

/**
 *  This represents a core type in the language
 *
 */
class CoreType : public QatType, public Definable {
public:
  /**
   *  Member represents an individual member of a core
   * type
   *
   */
  class Member {
  public:
    Member(std::string _name, QatType *_type, bool _variability,
           utils::VisibilityInfo _visibility)
        : name(_name), type(_type), variability(_variability),
          visibility(_visibility) {}

    /**
     *  Name of the member
     *
     */
    std::string name;

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
  std::string name;

  // Parent module
  QatModule *parent;

  // Member fields
  std::vector<Member *> members;

  // Static member fields
  std::vector<StaticMember *> static_members;

  // All member functions (non-static) of this type
  std::vector<MemberFunction *> memberFunctions;

  // All static member functions of this type
  std::vector<MemberFunction *> staticFunctions;

  // The destructor function of the core type
  std::optional<MemberFunction *> destructor;

  // VisibilityInfo of this CoreType
  utils::VisibilityInfo visibility;

  // TODO - Add support for extension functions

public:
  CoreType(QatModule *mod, const std::string _name,
           const std::vector<Member *> _members,
           const std::vector<MemberFunction *> _memberFunctions,
           const utils::VisibilityInfo _visibility);

  /**
   *  Get the full name of this type
   *
   * @return std::string
   */
  std::string getFullName() const;

  /**
   *  Get the name of this type only, without the prefix of the parent scopes
   */
  std::string getName() const;

  /**
   *  Get the index of the member field with the provided name
   *
   * @param member Name of the member to find
   * @return int Index of the corresponding field
   */
  int getIndexOf(const std::string member) const;

  unsigned getMemberCount() const;

  Member *getMemberAt(unsigned index);

  /**
   *  Get the name of the member field at the provided index
   *
   * @param index Index of the member field
   * @return std::string Name of the member corresponding to that field
   */
  std::string getMemberNameAt(const unsigned int index) const;

  QatType *getTypeOfMember(const std::string member) const;

  /**
   *  Whether there is a static member in this type with the provided
   * name
   *
   * @param name Name to check for
   * @return true If there is a static member
   * @return false If there is no static member
   */
  bool hasStatic(const std::string name) const;

  /**
   *  Whether there is a non-static member function of this type with the
   * provided name
   *
   * @param fnName Name of the function
   * @return true
   * @return false
   */
  bool hasMemberFunction(const std::string fnName) const;

  /**
   *  Get the member function of this type with the provided name
   *
   * @param fnName Name of the function
   * @return const IR::QatMemberFunction &
   */
  const MemberFunction *getMemberFunction(const std::string fnName) const;

  /**
   *  Whether there is a static member function of this type with the
   * provided name
   *
   * @param fnName Name of the function
   * @return true
   * @return false
   */
  bool hasStaticFunction(const std::string fnName) const;

  /**
   *  Get the static member function of this type with the provided name
   *
   * @param fnName Name of the function
   * @return const QatMemberFunction&
   */
  const MemberFunction *getStaticFunction(const std::string fnName) const;

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
  void addMemberFunction(const std::string name, const bool is_variation,
                         QatType *return_type, bool is_return_type_variable,
                         const bool is_async, const std::vector<Argument> args,
                         const bool has_variadic_args,
                         const utils::FileRange fileRange,
                         const utils::VisibilityInfo visib_info);

  // NOTE - Add documentation
  void addStaticFunction(const std::string name, QatType *return_type,
                         const bool is_return_type_variable,
                         const bool is_async, const std::vector<Argument> args,
                         const bool has_variadic_args,
                         const utils::FileRange fileRange,
                         const utils::VisibilityInfo visib_info);

  void addStaticMember(std::string name, QatType *type, bool variability,
                       Value *initial, utils::VisibilityInfo visibility);

  utils::VisibilityInfo getVisibility() const;

  QatModule *getParent();

  TypeKind typeKind() const;

  std::string toString() const;

  void defineLLVM(llvmHelper helper) const override;

  void defineCPP(cpp::File &file) const override;

  nuo::Json toJson() const override;
};

} // namespace qat::IR

#endif