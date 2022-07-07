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

  llvm::Type *emit(IR::Generator *generator);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  TypeKind typeKind();

  backend::JSON toJSON() const;
};
} // namespace AST
} // namespace qat

#endif