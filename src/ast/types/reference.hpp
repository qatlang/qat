#ifndef QAT_TYPES_REFERENCE_HPP
#define QAT_TYPES_REFERENCE_HPP

#include "./qat_type.hpp"
#include <string>

namespace qat::ast {

/**
 *  A reference type in the language
 *
 */
class ReferenceType : public QatType {
private:
  QatType *type;

public:
  /**
   *  Create a reference to the provided datatype
   *
   * @param _type Datatype to which the pointer is pointing to
   * @param _fileRange
   */
  ReferenceType(QatType *_type, const bool _variable,
                const utils::FileRange _fileRange);

  IR::QatType *emit(IR::Context *ctx);

  TypeKind typeKind();

  nuo::Json toJson() const;

  String toString() const;
};

} // namespace qat::ast

#endif