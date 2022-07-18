#ifndef QAT_IR_TYPES_POINTER_HPP
#define QAT_IR_TYPES_POINTER_HPP

#include "./qat_type.hpp"
#include <string>

namespace qat::IR {

/**
 *  A pointer type in the language
 *
 */
class PointerType : public QatType {
private:
  QatType *subType;

public:
  /**
   *  Create a pointer to the provided datatype
   *
   * @param _type Datatype to which the pointer is pointing to
   * @param _filePlacement
   */
  PointerType(QatType *_type);

  QatType *getSubType() const;

  TypeKind typeKind() const;

  std::string toString() const;
};

} // namespace qat::IR

#endif