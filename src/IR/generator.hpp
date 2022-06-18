#ifndef QAT_IR_GENERATOR_HPP
#define QAT_IR_GENERATOR_HPP

#include "../AST/box.hpp"
#include "../IO/qat_file.hpp"
#include "../utils/visibility.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include <string>
#include <vector>

namespace qat {
namespace IR {
/**
 * @brief The IR Generator for QAT
 */
class Generator {
private:
  IO::QatFile *unit;

  /**
   * @brief The name to be prefixed to all new global variables, functions,
   * boxes, libs and types
   *
   */
  std::string name_prefix;

public:
  /**
   * @brief The LLVMContext determines the context and scope of all
   * instructions, functions, blocks and variables
   */
  llvm::LLVMContext llvmContext;

  /**
   * @brief Provides Basic API to create instructions and then append
   * it to the end of a BasicBlock or to a specified location
   */
  llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>
      builder = llvm::IRBuilder(llvmContext, llvm::ConstantFolder(),
                                llvm::IRBuilderDefaultInserter(), nullptr,
                                llvm::None);

  /**
   * @brief All the boxes exposed in the current scope. This will automatically
   * be populated and de-populated when the expose scope starts and ends
   *
   */
  std::vector<qat::AST::Box> exposed;

  /**
   * @brief All visibilities of the symbols generated till a particular time
   * period. This is used to compound visibility. This is very helpful to manage
   * the public visibility.
   *
   */
  std::vector<utils::VisibilityKind> visibilities;

  /**
   * @brief Create a Function object pointer. This is not accompanying Function
   * AST as the module is a private variable
   *
   * @param name Name of the function
   * @param parameterTypes LLVM Type of the parameters of the function
   * @param returnType LLVM Type of the return value of the function
   * @param isVariableArguments Whether the function has variable number of
   * arguments
   * @param linkageType How to link the function symbol
   * @return llvm::Function*
   */
  llvm::Function *
  create_function(const std::string name,
                  const std::vector<llvm::Type *> parameterTypes,
                  llvm::Type *returnType, const bool isVariableArguments,
                  const llvm::GlobalValue::LinkageTypes linkageType);

  llvm::GlobalVariable *
  create_global_variable(const std::string name, llvm::Type *type,
                         llvm::Value *value, const bool is_constant,
                         const utils::VisibilityInfo access_info);

  llvm::Function *get_function(const std::string name);

  llvm::GlobalVariable *get_global_variable(const std::string name);

  bool does_function_exist(const std::string name);

  class AddResult {
  private:
    AddResult(bool _result, std::optional<std::string> _kind)
        : result(_result), kind(_kind) {}

  public:
    bool result;

    std::optional<std::string> kind;

    static AddResult True() { return AddResult(true, std::nullopt); }

    static AddResult False(std::string _kind) {
      return AddResult(false, _kind);
    }

    std::string getKind() { return kind.value_or(""); }
  };

  std::string get_name(std::string item);

  bool has_type(const std::string name, const bool full_name);

  AddResult add_type(const std::string self_name);

  void close_type();

  bool has_box(const std::string name, const bool full_name);

  AddResult add_box(const std::string self_name);

  void close_box();

  bool has_lib(const std::string name, const bool full_name);

  AddResult add_lib(const std::string self_name);

  void close_lib();

  void throw_error(const std::string message,
                   const utils::FilePlacement placement);
};
} // namespace IR
} // namespace qat

#endif