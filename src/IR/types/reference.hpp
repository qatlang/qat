#ifndef QAT_IR_TYPES_REFERENCE_HPP
#define QAT_IR_TYPES_REFERENCE_HPP

#include "./qat_type.hpp"
#include <string>

namespace qat {
namespace IR {

/**
 *  A reference type in the language
 *
 */
class ReferenceType : public QatType {
private:
  QatType *subType;

public:
  /**
   *  Create a reference to the provided datatype
   *
   * @param _type Datatype to which the pointer is pointing to
   * @param _filePlacement
   */
  ReferenceType(QatType *_type);

  QatType *getSubType() const;

  TypeKind typeKind() const;
};

} // namespace IR
} // namespace qat

#endif