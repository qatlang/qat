#ifndef QAT_AST_TYPES_TUPLE_HPP
#define QAT_AST_TYPES_TUPLE_HPP

#include "../../IR/generator.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/Type.h"
#include <vector>

namespace qat {
namespace AST {

/**
 *  Tuples are product types. It is a defined fixed-length sequence of
 * other types
 *
 */
class TupleType : public QatType {
private:
  std::vector<QatType *> types;

  // Whether this tuple should be packed
  bool isPacked;

public:
  TupleType(const std::vector<QatType *> _types, const bool _isPacked,
            const bool _variable, const utils::FilePlacement _filePlacement);

  /**
   *  Add another QatType to this tuple
   *
   * @param type
   */
  void add_type(QatType *type);

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