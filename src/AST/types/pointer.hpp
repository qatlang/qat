#ifndef QAT_TYPES_POINTER_HPP
#define QAT_TYPES_POINTER_HPP

#include "./qat_type.hpp"
#include "./void.hpp"
#include <string>

namespace qat::AST {

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
   * @param _filePlacement
   */
  PointerType(QatType *_type, const bool _variable,
              const utils::FilePlacement _filePlacement);

  IR::QatType *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  TypeKind typeKind();

  nuo::Json toJson() const;

  std::string toString() const;
};

} // namespace qat::AST

#endif