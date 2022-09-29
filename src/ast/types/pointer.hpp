#ifndef QAT_TYPES_POINTER_HPP
#define QAT_TYPES_POINTER_HPP

#include "./qat_type.hpp"
#include "./void.hpp"
#include <string>

namespace qat::ast {

/**
 *  A pointer type in the language
 *
 */
class PointerType : public QatType {
private:
  QatType *type;

public:
  /**
   *  Create a pointer to the provided datatype
   *
   * @param _type Datatype to which the pointer is pointing to
   * @param _fileRange
   */
  PointerType(QatType *_type, const bool _variable,
              const utils::FileRange _fileRange);

  IR::QatType *emit(IR::Context *ctx);

  TypeKind typeKind() const;

  Json toJson() const;

  String toString() const;
};

} // namespace qat::ast

#endif