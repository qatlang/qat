#ifndef QAT_TYPES_REFERENCE_HPP
#define QAT_TYPES_REFERENCE_HPP

#include "./qat_type.hpp"
#include "./void.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/raw_ostream.h"
#include <string>

namespace qat {
namespace AST {

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

  /**
   *  This is the code generator function that handles the generation of
   * LLVM IR
   *
   * @param generator The IR::Generator instance that handles LLVM IR Generation
   * @return llvm::Type*
   */
  llvm::Type *generate(IR::Generator *generator);

  /**
   *  TypeKind is used to detect variants of the QatType
   *
   * @return TypeKind
   */
  TypeKind typeKind();

  backend::JSON toJSON() const;
};

} // namespace AST
} // namespace qat

#endif