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
  IR::QatType *type;     // Type representation of the value
  Nature       nature;   // The nature of the value
  bool         variable; // Variability nature
  llvm::Value *ll;       // LLVM value

public:
  Value(llvm::Value *_llValue, IR::QatType *_type, bool _isVariable,
        Nature kind);

  useit QatType *getType() const; // Type of the value
  useit llvm::Value *getLLVM();
  useit bool         isReference() const;
  useit bool         isPointer() const;
  useit bool         isVariable() const;
  useit Nature       getNature() const;
};

} // namespace qat::IR

#endif