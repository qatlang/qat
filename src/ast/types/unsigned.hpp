#ifndef QAT_AST_TYPES_UNSIGNED_HPP
#define QAT_AST_TYPES_UNSIGNED_HPP

#include "../../IR/context.hpp"
#include "qat_type.hpp"

namespace qat::ast {

/**
 *  Unsigned integer datatype in the language
 *
 */
class UnsignedType : public QatType {
private:
  /**
   *  Bitwidth of the integer
   *
   */
  const u32 bitWidth;

public:
  /**
   *  Construct an Integer type with a bitwidth
   *
   * @param _bitWidth Bitwidth of the integer type
   * @param _variable Variability
   * @param _fileRange
   */
  UnsignedType(u64 _bitWidth, bool _variable, utils::FileRange _fileRange);

  IR::QatType *emit(IR::Context *ctx);

  TypeKind typeKind() const final;

  //  Whether the provided integer is the bitwidth of the UnsignedType
  bool isBitWidth(const u32 width) const;

  Json toJson() const;

  String toString() const;
};

} // namespace qat::ast

#endif