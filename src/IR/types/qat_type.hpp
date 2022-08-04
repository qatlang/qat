#ifndef QAT_IR_TYPES_QAT_TYPE_HPP
#define QAT_IR_TYPES_QAT_TYPE_HPP

#include "../../backend/cpp.hpp"
#include "../../utils/macros.hpp"
#include "../llvm_helper.hpp"
#include "../uniq.hpp"
#include "./type_kind.hpp"
#include "llvm/IR/Type.h"
#include <nuo/json.hpp>
#include <string>
#include <vector>

namespace qat::IR {

// QatType is the base class for all types in the IR
class QatType : public Uniq {
protected:
  static Vec<QatType *> types;

public:
  QatType();
  virtual ~QatType() = default;

  useit static bool         checkTypeExists(const String &name);
  useit bool                isSame(QatType *other) const;
  useit bool                isInteger() const;
  useit bool                isUnsignedInteger() const;
  useit bool                isFloat() const;
  useit bool                isReference() const;
  useit bool                isPointer() const;
  useit bool                isArray() const;
  useit bool                isTuple() const;
  useit bool                isFunction() const;
  useit bool                isTemplate() const;
  useit virtual TypeKind    typeKind() const                   = 0;
  useit virtual llvm::Type *emitLLVM(llvmHelper &helper) const = 0;
  virtual void              emitCPP(cpp::File &file) const     = 0;
  useit virtual String      toString() const                   = 0;
  useit virtual nuo::Json   toJson() const                     = 0;
};

} // namespace qat::IR

#endif