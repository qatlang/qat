#ifndef QAT_AST_TYPES_FLOAT_HPP
#define QAT_AST_TYPES_FLOAT_HPP

#include "./qat_type.hpp"
#include "cstdint"

namespace qat::AST {

/**
 *  FloatType represents a floating point number in the language
 *
 */
class FloatType : public QatType {
private:
  /**
   *  Kind of the floating point number
   *
   */
  IR::FloatTypeKind kind;

public:
  /**
   *  FloatType represents a floating point number in the language
   *
   * @param _kind Kind of the float
   * @param _filePlacement
   */
  FloatType(const IR::FloatTypeKind _kind, const bool _variable,
            const utils::FileRange _filePlacement);

  static std::string kindToString(IR::FloatTypeKind kind);

  IR::QatType *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  TypeKind typeKind();

  nuo::Json toJson() const;

  std::string toString() const;
};

} // namespace qat::AST

#endif