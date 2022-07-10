#ifndef QAT_TYPES_REFERENCE_HPP
#define QAT_TYPES_REFERENCE_HPP

#include "./qat_type.hpp"
#include <string>

namespace qat::AST {

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
   * @param _filePlacement
   */
  ReferenceType(QatType *_type, const bool _variable,
                const utils::FilePlacement _filePlacement);

  IR::QatType *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  TypeKind typeKind();

  backend::JSON toJSON() const;

  std::string toString() const;
};

} // namespace qat::AST

#endif