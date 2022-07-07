#ifndef QAT_AST_TYPES_INTEGER_HPP
#define QAT_AST_TYPES_INTEGER_HPP

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

public:
  /**
   *  Construct an Integer type with a bitwidth
   *
   * @param _bitWidth Bitwidth of the integer type
   * @param _filePlacement
   */
  IntegerType(const unsigned int _bitWidth, const bool _variable,
              const utils::FilePlacement _filePlacement);

  IR::QatType *emit(IR::Generator *generator);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  TypeKind typeKind();

  /**
   *  Whether the provided integer is the bitwidth of the IntegerType
   *
   * @param width Bitwidth to check for
   * @return true If the width matches
   * @return false If the width does not match
   */
  bool isBitWidth(const unsigned int width) const;

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif