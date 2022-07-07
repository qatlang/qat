#ifndef QAT_AST_TYPES_QAT_TYPE_HPP
#define QAT_AST_TYPES_QAT_TYPE_HPP

#include "../../IR/generator.hpp"
#include "../../backend/cpp.hpp"
#include "../../backend/json.hpp"
#include "../../utils/file_placement.hpp"
#include "./type_kind.hpp"
#include "llvm/IR/Type.h"
#include <string>

namespace qat {
namespace AST {

/**
 *  This is the base class representing a type in the language
 *
 */
class QatType {
private:
  bool variable;

public:
  QatType(const bool _variable, const utils::FilePlacement _filePlacement)
      : variable(_variable), filePlacement(_filePlacement) {}

  virtual ~QatType(){};

  // This is the code generator function that handles the generation of
  // LLVM IR
  virtual llvm::Type *emit(IR::Generator *generator){};

  // TypeKind is used to detect variants of the QatType
  virtual TypeKind typeKind(){};

  // Whether this type was accompanied by the var keyword, which
  // represents variability for the value in context
  bool isVariable() const { return variable; }

  bool isConstant() const { return !variable; }

  // This is the generator function for C++
  virtual void emitCPP(backend::cpp::File &file, bool isHeader) const {};

  // This generates JSON to represent the type
  virtual backend::JSON toJSON() const {};

  // FilePlacement representing the range in the file this type was
  // parsed from
  utils::FilePlacement filePlacement;
};
} // namespace AST
} // namespace qat

#endif