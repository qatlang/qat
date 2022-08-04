#ifndef QAT_AST_TYPES_VOID_HPP
#define QAT_AST_TYPES_VOID_HPP

#include "./qat_type.hpp"

#include <string>

namespace qat::ast {

/**
 *  Void type in the language
 *
 */
class VoidType : public QatType {
public:
  /**
   *  Void type in the language
   *
   * @param _fileRange
   */
  VoidType(const bool _variable, const utils::FileRange _fileRange);

  IR::QatType *emit(IR::Context *ctx);

  TypeKind typeKind();

  nuo::Json toJson() const;

  String toString() const;
};

} // namespace qat::ast

#endif