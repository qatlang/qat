#ifndef QAT_AST_TYPES_UNSIGNED_HPP
#define QAT_AST_TYPES_UNSIGNED_HPP

#include "../../IR/context.hpp"
#include "qat_type.hpp"

namespace qat {
namespace AST {

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
  const unsigned int bitWidth;

public:
  /**
   *  Construct an Integer type with a bitwidth
   *
   * @param _bitWidth Bitwidth of the integer type
   * @param _variable Variability
   * @param _filePlacement
   */
  UnsignedType(const unsigned int _bitWidth, const bool _variable,
               const utils::FilePlacement _filePlacement);

  IR::QatType *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  TypeKind typeKind();

  //  Whether the provided integer is the bitwidth of the UnsignedType
  bool isBitWidth(const unsigned int width) const;

  backend::JSON toJSON() const;

  std::string toString() const;
};
} // namespace AST
} // namespace qat

#endif