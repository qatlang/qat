#ifndef QAT_AST_TYPES_VOID_HPP
#define QAT_AST_TYPES_VOID_HPP

#include "../../IR/generator.hpp"
#include "qat_type.hpp"
#include "llvm/IR/Type.h"
#include <string>

namespace qat {
namespace AST {

/**
 *  Void type in the language
 *
 */
class VoidType : public QatType {
public:
  /**
   *  Void type in the language
   *
   * @param _filePlacement
   */
  VoidType(const bool _variable, const utils::FilePlacement _filePlacement);

  llvm::Type *emit(IR::Generator *generator);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  TypeKind typeKind();

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif