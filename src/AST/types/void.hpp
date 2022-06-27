#ifndef QAT_AST_TYPES_VOID_HPP
#define QAT_AST_TYPES_VOID_HPP

#include "../../IR/generator.hpp"
#include "qat_type.hpp"
#include "llvm/IR/Type.h"
#include <string>

namespace qat {
namespace AST {

/**
 *  Void type in the language
 *
 */
class VoidType : public QatType {
public:
  /**
   *  Void type in the language
   *
   * @param _filePlacement
   */
  VoidType(const bool _variable, const utils::FilePlacement _filePlacement);

  /**
   *  This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Type*
   */
  llvm::Type *generate(IR::Generator *generator);

  /**
   *  TypeKind is used to detect variants of the QatType
   *
   * @return TypeKind
   */
  TypeKind typeKind();

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif