#ifndef QAT_AST_TYPES_FLOAT_HPP
#define QAT_AST_TYPES_FLOAT_HPP

#include "./qat_type.hpp"
#include "cstdint"

namespace qat::ast {

/**
 *  FloatType represents a floating point number in the language
 *
 */
class FloatType : public QatType {
private:
  /**
   *  Nature of the floating point number
   *
   */
  IR::FloatTypeKind kind;

public:
  /**
   *  FloatType represents a floating point number in the language
   *
   * @param _kind Nature of the float
   * @param _fileRange
   */
  FloatType(const IR::FloatTypeKind _kind, const bool _variable,
            const utils::FileRange _fileRange);

  static String kindToString(IR::FloatTypeKind kind);

  IR::QatType *emit(IR::Context *ctx);

  TypeKind typeKind();

  nuo::Json toJson() const;

  String toString() const;
};

} // namespace qat::ast

#endif