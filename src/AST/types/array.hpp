#ifndef QAT_AST_TYPES_ARRAY_HPP
#define QAT_AST_TYPES_ARRAY_HPP

#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat {
namespace AST {

/**
 * @brief A continuous sequence of elements of a particular type. The sequence
 * is fixed length
 *
 */
class ArrayType : public QatType {
private:
  /**
   * @brief Type of the element of the array
   *
   */
  QatType element_type;

  /**
   * @brief The length of the array (number of items in it)
   *
   */
  uint64_t length;

public:
  /**
   * @brief ArrayType represents an array of elements of the provided type
   *
   * @param _element_type
   * @param _length
   * @param _filePlacement
   */
  ArrayType(QatType _element_type, const uint64_t _length, const bool _variable,
            const utils::FilePlacement _filePlacement);

  /**
   * @brief This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Type*
   */
  llvm::Type *generate(IR::Generator *generator);

  /**
   * @brief TypeKind is used to detect variants of the QatType
   *
   * @return TypeKind
   */
  TypeKind typeKind();
};

} // namespace AST
} // namespace qat

#endif