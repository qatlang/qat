#ifndef QAT_IR_VALUE_HPP
#define QAT_IR_VALUE_HPP

#include "../backend/cpp.hpp"
#include "../backend/json.hpp"
#include "llvm_helper.hpp"
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace IR {

class QatType;

enum class Kind { assignable, temporary, pure, expired };

class Value {
protected:
  // Type representation of the value
  IR::QatType *type;

  // The kind/nature of the value
  Kind kind;

  // Variability nature
  bool variable;

public:
  Value(IR::QatType *_type, bool _isVariable, Kind kind);

  const IR::QatType *getType() const;

  bool isReference() const;

  bool isPointer() const;

  bool isVariable() const;

  Kind getKind() const;

  virtual llvm::Value *emitLLVM(llvmHelper helper) const {}

  virtual void emitCPP(backend::cpp::File &file) const {}

  virtual backend::JSON toJSON() const {}
};

} // namespace IR
} // namespace qat

#endif