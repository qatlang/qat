#ifndef QAT_IR_TYPES_CORE_TYPE_HPP
#define QAT_IR_TYPES_CORE_TYPE_HPP

#include "../../utils/visibility.hpp"
#include "../member_function.hpp"
#include "../static_member.hpp"
#include "./qat_type.hpp"
#include <string>
#include <vector>

namespace qat {
namespace IR {

/**
 *  This represents a core type in the language
 *
 */
class CoreType : public QatType {
public:
  /**
   *  Member represents an individual member of a core
   * type
   *
   */
  class Member {
  public:
    Member(std::string _name, QatType *_type, utils::VisibilityInfo _visibility)
        : name(_name), type(_type), visibility(_visibility) {}

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
  std::string name;

  // Parent module
  QatModule *parent;

  /**
   *  Member fields
   *
   */
  std::vector<Member *> members;

  /**
   *  Static member fields
   *
   */
  std::vector<StaticMember *> static_members;

  /**
   *  All member functions (non-static) of this type
   *
   */
  std::vector<MemberFunction *> memberFunctions;

  /**
   *  All static member functions of this type
   *
   */
  std::vector<MemberFunction *> staticFunctions;

  /**
   *  The destructor function of the core type
   *
   */
  std::optional<MemberFunction *> destructor;

  // VisibilityInfo of this CoreType
  utils::VisibilityInfo visibility;

  // TODO - Add support for extension functions

public:
  CoreType(llvm::LLVMContext &ctx, llvm::Module *mod, const std::string _name,
           const std::vector<Member *> _members,
           const std::vector<MemberFunction *> _memberFunctions,
           const bool _isPacked, const utils::VisibilityInfo _visibility);

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
  int get_index_of(const std::string member) const;

  unsigned getMemberCount() const;

  Member *getMemberAt(unsigned index);

  /**
   *  Get the name of the member field at the provided index
   *
   * @param index Index of the member field
   * @return std::string Name of the member corresponding to that field
   */
  std::string getMemberNameAt(const unsigned int index) const;

  QatType *get_type_of_member(const std::string member) const;

  /**
   *  Whether there is a static member in this type with the provided
   * name
   *
   * @param name Name to check for
   * @return true If there is a static member
   * @return false If there is no static member
   */
  bool has_static(const std::string name) const;

  /**
   *  Whether there is a non-static member function of this type with the
   * provided name
   *
   * @param fnName Name of the function
   * @return true
   * @return false
   */
  bool has_member_function(const std::string fnName) const;

  /**
   *  Get the member function of this type with the provided name
   *
   * @param fnName Name of the function
   * @return const qat::IR::QatMemberFunction &
   */
  const MemberFunction *get_member_function(const std::string fnName) const;

  /**
   *  Whether there is a static member function of this type with the
   * provided name
   *
   * @param fnName Name of the function
   * @return true
   * @return false
   */
  bool has_static_function(const std::string fnName) const;

  /**
   *  Get the static member function of this type with the provided name
   *
   * @param fnName Name of the function
   * @return const QatMemberFunction&
   */
  const MemberFunction *get_static_function(const std::string fnName) const;

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
  void add_member_function(const std::string name, const bool is_variation,
                           QatType *return_type, bool is_return_type_variable,
                           const bool is_async,
                           const std::vector<Argument> args,
                           const bool has_variadic_args,
                           const utils::FilePlacement placement,
                           const utils::VisibilityInfo visib_info);

  // NOTE - Add documentation
  void add_static_function(const std::string name, QatType *return_type,
                           const bool is_return_type_variable,
                           const bool is_async,
                           const std::vector<Argument> args,
                           const bool has_variadic_args,
                           const utils::FilePlacement placement,
                           const utils::VisibilityInfo visib_info);

  utils::VisibilityInfo getVisibility() const;

  QatModule *getParent();

  TypeKind typeKind() const;

  std::string toString() const;
};

} // namespace IR
} // namespace qat

#endif