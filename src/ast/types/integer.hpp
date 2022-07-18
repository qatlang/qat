#ifndef QAT_AST_TYPES_INTEGER_HPP
#define QAT_AST_TYPES_INTEGER_HPP

#include "../../IR/context.hpp"
#include "qat_type.hpp"

namespace qat::ast {

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
   * @param _fileRange
   */
  IntegerType(const unsigned int _bitWidth, const bool _variable,
              const utils::FileRange _fileRange);

  IR::QatType *emit(IR::Context *ctx);

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

  nuo::Json toJson() const;

  std::string toString() const;
};

} // namespace qat::ast

#endif