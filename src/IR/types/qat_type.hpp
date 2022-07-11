#ifndef QAT_IR_TYPES_QAT_TYPE_HPP
#define QAT_IR_TYPES_QAT_TYPE_HPP

#include "../../backend/cpp.hpp"
#include "../../backend/json.hpp"
#include "../llvm_helper.hpp"
#include "./type_kind.hpp"
#include "llvm/IR/Type.h"
#include <string>
#include <vector>

namespace qat::IR {

// QatType is the base class for all types in the IR
class QatType {
protected:
  static std::vector<QatType *> types;

public:
  QatType();

  virtual ~QatType(){};

  // TypeKind is used to detect variants of the QatType
  virtual TypeKind typeKind() const {}

  static bool checkTypeExists(const std::string name);

  bool isSame(QatType *other) const;

  // Emitters

  virtual llvm::Type *emitLLVM(llvmHelper helper) const {}

  virtual void emitCPP(backend::cpp::File &file) const {}

  // Converters

  virtual std::string toString() const {}

  virtual backend::JSON toJSON() const {}
};

} // namespace qat::IR

#endif