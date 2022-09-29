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
  ReferenceType(QatType *_type, bool _variable, utils::FileRange _fileRange);

  useit IR::QatType *emit(IR::Context *ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif