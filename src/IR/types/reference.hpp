#ifndef QAT_IR_TYPES_REFERENCE_HPP
#define QAT_IR_TYPES_REFERENCE_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"
#include <string>

namespace qat::IR {

class ReferenceType : public QatType {
private:
  QatType* subType;
  bool     isSubVariable;
  ReferenceType(bool isSubtypeVariable, QatType* _type, IR::Context* ctx);

public:
  useit static ReferenceType* get(bool _isSubtypeVariable, QatType* _subtype, IR::Context* ctx);

  useit QatType* getSubType() const;
  useit bool     isSubtypeVariable() const;
  useit bool     isTypeSized() const final;
  useit TypeKind typeKind() const final;
  useit String   toString() const final;
};

} // namespace qat::IR

#endif