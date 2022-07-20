#ifndef QAT_AST_TYPES_ARRAY_HPP
#define QAT_AST_TYPES_ARRAY_HPP

#include "./qat_type.hpp"

namespace qat::ast {

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
   * @param _fileRange
   */
  ArrayType(QatType *_element_type, const uint64_t _length,
            const bool _variable, const utils::FileRange _fileRange);

  IR::QatType *emit(IR::Context *ctx);

  TypeKind typeKind();

  nuo::Json toJson() const;

  std::string toString() const;
};

} // namespace qat::ast

#endif