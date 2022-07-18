#ifndef QAT_AST_SENTENCES_LOCAL_DECLARATION_HPP
#define QAT_AST_SENTENCES_LOCAL_DECLARATION_HPP

#include "../expression.hpp"
#include "../sentence.hpp"
#include "../types/qat_type.hpp"

#include <optional>

namespace qat::AST {

/**
 *  LocalDeclaration represents declaration of values or variables inside
 * functions
 *
 */
class LocalDeclaration : public Sentence {
private:
  /**
   *  Optional QatType instance representing the type of the variable.
   * This is optional so as for type inference.
   *
   */
  QatType *type;

  /**
   *  Name of the entity
   *
   */
  std::string name;

  /**
   *  Value to assign to the entity
   *
   */
  Expression *value;

  /**
   *  Whether this entity is a variable or not
   *
   */
  bool variability;

public:
  /**
   *  LocalDeclaration represents declaration of variables inside
   * functions
   *
   * @param _type The optional type of the entity. If this is optional, the type
   * should be inferred.
   * @param _name Name of the entity
   * @param _value Value to be stored into the entity
   * @param _variability Whether the entity is a variable or not
   * @param _filePlacement
   */
  LocalDeclaration(QatType *_type, std::string _name, Expression *_value,
                   bool _variability, utils::FilePlacement _filePlacement);

  /**
   *  Set the origin block of the declaration
   *
   * @param ctx The LLVMContext
   * @param alloca The alloca instruction related to the declaration
   * @param bb The BasicBlock in which the declaration occured
   */
  void set_origin_block(llvm::LLVMContext &ctx, llvm::AllocaInst *alloca,
                        llvm::BasicBlock *bb);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::localDeclaration; }
};

} // namespace qat::AST

#endif