#ifndef QAT_IR_FUNCTION_HPP
#define QAT_IR_FUNCTION_HPP

#include "../utils/file_placement.hpp"
#include "../utils/visibility.hpp"
#include "./argument.hpp"
#include "./block.hpp"
#include "types/qat_type.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace qat {
namespace IR {

class QatModule;

// Function represents a normal function in the language
class Function : public Value {
protected:
  // Name of the function
  std::string name;

  bool isReturnValueVariable;

  // Parent module of the function
  QatModule *mod;

  // Arguments of the function
  std::vector<Argument> arguments;

  // Information about the visibility of this function
  utils::VisibilityInfo visibility_info;

  // FilePlacement of this function
  utils::FilePlacement placement;

  //  Whether this function is async or not
  bool is_async;

  bool has_variadic_args;

  // All blocks in the function
  std::vector<Block *> blocks;

  // Current block
  Block *current;

  // Number of calls made to this function
  unsigned calls;

  unsigned refers;

  // Private constructor for Function
  Function(QatModule *mod, std::string _name,
           QatType *returnType, bool _isReturnValueVariable, bool _is_async,
           std::vector<Argument> _args, bool has_variadic_arguments,
           utils::FilePlacement placement,
           utils::VisibilityInfo _visibility_info);

public:
  // Create a member function for the provided parent type
  static Function *Create(QatModule *mod,
                          const std::string name, QatType *return_type,
                          bool isReturnValueVariable, bool is_async,
                          const std::vector<Argument> args,
                          const bool has_variadic_args,
                          const utils::FilePlacement placement,
                          const utils::VisibilityInfo visib_info);

  virtual bool isMemberFunction() const;

  //  Whether this function has variadic arguments
  bool hasVariadicArgs() const;

  // Whether this function is async or not
  bool isAsyncFunction() const;

  // The name of the argument at the provided index
  std::string argumentNameAt(unsigned int index) const;

  // Short name of the function, without the prefix of the parent
  std::string getName() const;

  // Full name of the function, with the name of the parent prefixed
  virtual std::string getFullName() const;

  // Whether this function returns reference to another entity
  bool isReturnTypeReference() const;

  // Whether this function returns data of pointer type
  bool isReturnTypePointer() const;

  // Whether this function is accessible by the requester
  bool isAccessible(const utils::RequesterInfo req_info) const;

  Block *getEntryBlock();

  // Add a new block
  Block *addBlock(bool isSub);

  // Get the current block
  Block *getCurrentBlock();

  // Set the current block
  void setCurrentBlock(Block *block);

  // Get the visibility of this function
  utils::VisibilityInfo getVisibility() const;
};

} // namespace IR
} // namespace qat

#endif