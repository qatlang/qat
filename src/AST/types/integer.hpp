#ifndef QAT_AST_TYPES_INT_HPP
#define QAT_AST_TYPES_INT_HPP

#include "../../IR/generator.hpp"
#include "qat_type.hpp"
#include "llvm/IR/Type.h"

namespace qat {
namespace AST {

/**
 *  Integer datatype in the language
 *
 */
class IntegerType : public QatType {
private:
  /**
   *  Bitwidth of the integer
   *
   */
  const unsigned int bitWidth;

  const bool isUnsigned;

public:
  /**
   *  Construct an Integer type with a bitwidth
   *
   * @param _bitWidth Bitwidth of the integer type
   * @param _filePlacement
   */
  IntegerType(const unsigned int _bitWidth, const bool isUnsigned,
              const bool _variable, const utils::FilePlacement _filePlacement);

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

  /**
   *  Whether the provided integer is the bitwidth of the IntegerType
   *
   * @param width Bitwidth to check for
   * @return true If the width matches
   * @return false If the width does not match
   */
  bool isBitWidth(const unsigned int width) const;

  bool isUnsignedType() const noexcept;

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif