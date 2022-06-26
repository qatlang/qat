#ifndef QAT_AST_TYPES_QAT_TYPE_HPP
#define QAT_AST_TYPES_QAT_TYPE_HPP

#include "../../IR/generator.hpp"
#include "../../utils/file_placement.hpp"
#include "./type_kind.hpp"
#include "llvm/IR/Type.h"
#include <string>

namespace qat {
namespace AST {

/**
 *  This is the base class representing a type in the language
 *
 */
class QatType {
private:
  bool variable;

public:
  QatType(const bool _variable, const utils::FilePlacement _filePlacement)
      : variable(_variable), filePlacement(_filePlacement) {}

  virtual ~QatType(){};

  /**
   *  This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Type*
   */
  virtual llvm::Type *generate(IR::Generator *generator){};

  /**
   *  TypeKind is used to detect variants of the QatType
   *
   * @return TypeKind
   */
  virtual TypeKind typeKind(){};

  /**
   *  Whether this type was accompanied by the var keyword, which
   * represents variability for the value in context
   *
   * @return true If the value is supposed to be a variable
   * @return false If the value is NOT supposed to be a variable
   */
  bool isVariable() { return variable; }

  /**
   *  FilePlacement representing the range in the file this type was
   * parsed from
   *
   */
  utils::FilePlacement filePlacement;
};
} // namespace AST
} // namespace qat

#endif