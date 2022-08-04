#ifndef QAT_IR_VALUE_HPP
#define QAT_IR_VALUE_HPP

#include "../backend/cpp.hpp"
#include "llvm_helper.hpp"
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include <nuo/json.hpp>

namespace qat::IR {

class QatType;

enum class Nature { assignable, temporary, pure, expired };

class Value {
protected:
  // Type representation of the value
  IR::QatType *type;
  // The nature of the value
  Nature nature;
  // Variability nature
  bool variable;

public:
  Value(IR::QatType *_type, bool _isVariable, Nature kind);

  useit QatType          *getType() const; // Type of the value
  useit bool              isReference() const;
  useit bool              isPointer() const;
  useit bool              isVariable() const;
  useit Nature            getKind() const;
  virtual llvm::Value    *emitLLVM(llvmHelper &helper) const = 0;
  virtual void            emitCPP(cpp::File &file) const     = 0;
  useit virtual nuo::Json toJson() const                     = 0;
};

} // namespace qat::IR

#endif