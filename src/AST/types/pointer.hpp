#ifndef QAT_TYPES_POINTER_HPP
#define QAT_TYPES_POINTER_HPP

#include "../../utils/llvm_type_to_name.hpp"
#include "./qat_type.hpp"
#include "./void.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

namespace qat {
namespace AST {

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

  IR::QatType *emit(IR::Generator *generator);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  TypeKind typeKind();

  backend::JSON toJSON() const;
};

} // namespace AST
} // namespace qat

#endif