#ifndef QAT_AST_TYPES_FLOAT_HPP
#define QAT_AST_TYPES_FLOAT_HPP

#include "./qat_type.hpp"
#include "cstdint"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace AST {

/**
 *  The kind of floating point number
 *
 */
enum class FloatTypeKind { _brain, _half, _32, _64, _80, _128PPC, _128 };

std::string to_string(FloatTypeKind);

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
  FloatTypeKind kind;

public:
  /**
   *  FloatType represents a floating point number in the language
   *
   * @param _kind Kind of the float
   * @param _filePlacement
   */
  FloatType(const FloatTypeKind _kind, const bool _variable,
            const utils::FilePlacement _filePlacement);

  static std::string kindToString(FloatTypeKind kind);

  llvm::Type *emit(IR::Generator *generator);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  TypeKind typeKind();

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif