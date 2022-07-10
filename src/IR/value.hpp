#ifndef QAT_IR_VALUE_HPP
#define QAT_IR_VALUE_HPP

#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

namespace qat {
namespace IR {

class QatType;

enum class Kind { assignable, temporary, pure, expired };

class Value {
private:
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

  virtual llvm::Value *
  emitLLVM(llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>
               builder) const {}
};

} // namespace IR
} // namespace qat

#endif