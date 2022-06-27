#ifndef QAT_AST_TYPES_FLOAT_HPP
#define QAT_AST_TYPES_FLOAT_HPP

#include "./qat_type.hpp"
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