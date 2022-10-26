#ifndef QAT_IR_TYPES_QAT_TYPE_HPP
#define QAT_IR_TYPES_QAT_TYPE_HPP

#include "../../backend/cpp.hpp"
#include "../../utils/json.hpp"
#include "../../utils/macros.hpp"
#include "../llvm_helper.hpp"
#include "../uniq.hpp"
#include "./type_kind.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include <string>
#include <vector>

namespace qat::IR {

class IntegerType;
class UnsignedType;
class FloatType;
class ReferenceType;
class PointerType;
class ArrayType;
class TupleType;
class FunctionType;
class StringSliceType;
class CStringType;
class CoreType;
class DefinitionType;
class MixType;
class ChoiceType;
class FutureType;
class MaybeType;

// QatType is the base class for all types in the IR
class QatType : public Uniq {
protected:
  static Vec<QatType*> types;
  llvm::Type*          llvmType;

public:
  QatType();
  virtual ~QatType() = default;

  useit static bool      checkTypeExists(const String& name);
  useit bool             isSame(QatType* other);
  useit bool             isDefinition() const;
  useit DefinitionType*  asDefinition() const;
  useit bool             isInteger() const;
  useit IntegerType*     asInteger() const;
  useit bool             isUnsignedInteger() const;
  useit UnsignedType*    asUnsignedInteger() const;
  useit bool             isFloat() const;
  useit FloatType*       asFloat() const;
  useit bool             isReference() const;
  useit ReferenceType*   asReference() const;
  useit bool             isPointer() const;
  useit PointerType*     asPointer() const;
  useit bool             isArray() const;
  useit ArrayType*       asArray() const;
  useit bool             isTuple() const;
  useit TupleType*       asTuple() const;
  useit bool             isFunction() const;
  useit FunctionType*    asFunction() const;
  useit bool             isCoreType() const;
  useit CoreType*        asCore() const;
  useit bool             isMix() const;
  useit MixType*         asMix() const;
  useit bool             isChoice() const;
  useit ChoiceType*      asChoice() const;
  useit bool             isStringSlice() const;
  useit StringSliceType* asStringSlice() const;
  useit bool             isCString() const;
  useit CStringType*     asCString() const;
  useit bool             isFuture() const;
  useit FutureType*      asFuture() const;
  useit bool             isMaybe() const;
  useit MaybeType*       asMaybe() const;
  useit bool             isVoid() const;
  useit virtual TypeKind typeKind() const = 0;
  useit virtual String   toString() const = 0;
  useit virtual Json     toJson() const   = 0;
  useit llvm::Type* getLLVMType() const { return llvmType; }
};

} // namespace qat::IR

#endif