#ifndef QAT_IR_MEMBER_FUNCTION_HPP
#define QAT_IR_MEMBER_FUNCTION_HPP

#include "../utils/visibility.hpp"
#include "./argument.hpp"
#include "./function.hpp"

#include <string>
#include <vector>

namespace qat::IR {

class CoreType;

/**
 *  MemberFunction represents a member function for a core type in
 * the language. It can be static or non-static
 *
 */
class MemberFunction : public Function {
private:
  /**
   *  Parent type
   *
   */
  CoreType *parent;

  //  Whether this is a static member function or not
  bool isStatic;

  bool isVariation;

  /**
   *  Private constructor for MemberFunction
   *
   * @param mod LLVM Module to insert the generated function to
   * @param _name Name of the function
   * @param returnType LLVM Type of the return value
   * @param _arg_names Names of arguments
   * @param _arg_types LLVM Types of the arguments
   * @param _args_variability Variability values of the arguments
   * @param has_variadic_arguments Whether the function has variable number of
   * arguments
   * @param _is_static Whether the function is a static member function
   * @param _visibility_info VisibilityInfo of the function
   */
  MemberFunction(bool _isVariation, CoreType *_parent, std::string _name,
                 QatType *returnType, bool isReturnTypeVariable, bool _is_async,
                 std::vector<Argument> _args, bool has_variadic_arguments,
                 bool _is_static, utils::FileRange _fileRange,
                 utils::VisibilityInfo _visibility_info);

public:
  /**
   *  Create a member function for the provided parent type
   *
   * @param mod LLVM Module to insert the generated LLVM Function to
   * @param parent LLVM Type of the parent core type
   * @param is_variation Whether this is a variation or not
   * @param name Name of the function (short)
   * @param return_type LLVM Type of the return type
   * @param args Arguments of the function
   * @param has_variadic_args Whether the function has variadic arguments
   * @param visib_info VisibilityInfo of the function
   * @return MemberFunction
   */
  static MemberFunction *
  Create(CoreType *parent, const bool is_variation, const std::string name,
         QatType *return_type, const bool isReturnTypeVariable,
         const bool is_async, const std::vector<Argument> args,
         const bool has_variadic_args, const utils::FileRange fileRange,
         const utils::VisibilityInfo visib_info);

  static MemberFunction *CreateDestructor(CoreType *parent,
                                          const utils::FileRange fileRange);

  /**
   *  Create a static member function for the provided parent type
   *
   * @param mod LLVM Module to insert the generated LLVM function to
   * @param parent LLVM Type of the parent core type
   * @param name Name of the function (short)
   * @param return_type LLVM Type of the return type
   * @param args Arguments of the function
   * @param has_variadic_args Whether the function has variadic arguments
   * @param visib_info VisibilityInfo of the function
   * @return MemberFunction
   */
  static MemberFunction *
  CreateStatic(CoreType *parent, const std::string name, QatType *return_type,
               const bool is_return_type_variable, const bool is_async,
               const std::vector<Argument> args, const bool has_variadic_args,
               const utils::FileRange fileRange,
               const utils::VisibilityInfo visib_info);

  /**
   *  Whether this function is a variation or not. This only applies to
   * non-static member functions
   *
   * @return true
   * @return false
   */
  bool isVariationFunction() const;

  bool isStaticFunction() const;

  std::string getFullName() const;

  bool isMemberFunction() const;
};

} // namespace qat::IR

#endif