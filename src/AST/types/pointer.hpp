#ifndef QAT_TYPES_POINTER_HPP
#define QAT_TYPES_POINTER_HPP

#include "../../show.hpp"
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
};

} // namespace AST
} // namespace qat

#endif