#ifndef QAT_AST_TYPES_ARRAY_HPP
#define QAT_AST_TYPES_ARRAY_HPP

#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat {
namespace AST {

/**
 *  A continuous sequence of elements of a particular type. The sequence
 * is fixed length
 *
 */
class ArrayType : public QatType {
private:
  /**
   *  Type of the element of the array
   *
   */
  QatType *element_type;

  /**
   *  The length of the array (number of items in it)
   *
   */
  uint64_t length;

public:
  /**
   *  ArrayType represents an array of elements of the provided type
   *
   * @param _element_type
   * @param _length
   * @param _filePlacement
   */
  ArrayType(QatType *_element_type, const uint64_t _length,
            const bool _variable, const utils::FilePlacement _filePlacement);

  IR::QatType *emit(IR::Generator *generator);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  TypeKind typeKind();

  backend::JSON toJSON() const;
};

} // namespace AST
} // namespace qat

#endif