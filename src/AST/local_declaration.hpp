#ifndef QAT_AST_LOCAL_DECLARATION_HPP
#define QAT_AST_LOCAL_DECLARATION_HPP

#include "../IR/generator.hpp"
#include "../utils/llvm_type_to_name.hpp"
#include "../utils/pointer_kind.hpp"
#include "../utils/variability.hpp"
#include "./types/qat_type.hpp"
#include "expression.hpp"
#include "sentence.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include <optional>

namespace qat {
namespace AST {

/**
 * @brief LocalDeclaration represents declaration of values or variables inside
 * functions
 *
 */
class LocalDeclaration : public Sentence {
private:
  /**
   * @brief Optional QatType instance representing the type of the variable.
   * This is optional so as for type inference.
   *
   */
  std::optional<QatType> type;

  /**
   * @brief Name of the entity
   *
   */
  std::string name;

  /**
   * @brief Value to assign to the entity
   *
   */
  Expression value;

  /**
   * @brief Whether this entity is a variable or not
   *
   */
  bool variability;

public:
  /**
   * @brief LocalDeclaration represents declaration of variables inside
   * functions
   *
   * @param _type The optional type of the entity. If this is optional, the type
   * should be inferred.
   * @param _name Name of the entity
   * @param _value Value to be stored into the entity
   * @param _variability Whether the entity is a variable or not
   * @param _filePlacement
   */
  LocalDeclaration(std::optional<QatType> _type, std::string _name,
                   Expression _value, bool _variability,
                   utils::FilePlacement _filePlacement);

  /**
   * @brief Set the origin block of the declaration
   *
   * @param ctx The LLVMContext
   * @param alloca The alloca instruction related to the declaration
   * @param bb The BasicBlock in which the declaration occured
   */
  void set_origin_block(llvm::LLVMContext &ctx, llvm::AllocaInst *alloca,
                        llvm::BasicBlock *bb);

  /**
   * @brief This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Value*
   */
  llvm::Value *generate(IR::Generator *generator);

  /**
   * @brief Type of the node represented by this AST member
   *
   * @return NodeType
   */
  NodeType nodeType() { return NodeType::localDeclaration; }
};

} // namespace AST
} // namespace qat

#endif