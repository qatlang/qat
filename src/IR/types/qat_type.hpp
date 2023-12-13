#ifndef QAT_IR_TYPES_QAT_TYPE_HPP
#define QAT_IR_TYPES_QAT_TYPE_HPP

#include "../../utils/json.hpp"
#include "../../utils/macros.hpp"
#include "../link_names.hpp"
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
class CoreType;
class DefinitionType;
class MixType;
class ChoiceType;
class FutureType;
class MaybeType;
class Region;
class ExpandedType;
class Context;
class Function;
class Value;
class OpaqueType;
class TypedType;
class PrerunValue;
class CType;
class ResultType;
class DoSkill;

// QatType is the base class for all types in the IR
class QatType : public Uniq {
protected:
  String               linkingName;
  static Vec<QatType*> types;
  llvm::Type*          llvmType;
  Vec<DoSkill*>        doneSkills;

public:
  QatType();
  virtual ~QatType() = default;
  static void clearAll();

  useit bool                  isTypeDoneByDefault() const;
  useit virtual bool          hasNoValueSemantics() const;
  useit virtual bool          canBePrerunGeneric() const;
  useit virtual Maybe<String> toPrerunGenericString(IR::PrerunValue* val) const;
  useit virtual bool          isTypeSized() const;
  useit virtual Maybe<bool>   equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const;
  useit String                getNameForLinking() const;

  useit static bool         checkTypeExists(const String& name);
  useit static Vec<Region*> allRegions();
  useit bool                isSame(QatType* other);
  useit bool                isCompatible(QatType* other);

  useit virtual bool  isExpanded() const;
  useit ExpandedType* asExpanded() const;

  useit virtual bool isCopyConstructible() const;
  useit virtual bool isCopyAssignable() const;
  useit virtual bool isMoveConstructible() const;
  useit virtual bool isMoveAssignable() const;
  useit virtual bool isDestructible() const;
  useit virtual bool isTriviallyCopyable() const;
  useit virtual bool isTriviallyMovable() const;

  virtual void copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun);
  virtual void copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun);
  virtual void moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun);
  virtual void moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun);
  virtual void destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun);

  useit bool        isOpaque() const;
  useit OpaqueType* asOpaque() const;

  useit bool            isDefinition() const;
  useit DefinitionType* asDefinition() const;

  useit bool         isInteger() const;
  useit IntegerType* asInteger() const;

  useit bool          isUnsignedInteger() const;
  useit UnsignedType* asUnsignedInteger() const;

  useit bool          isBool() const;
  useit UnsignedType* asBool() const;

  useit bool       isFloat() const;
  useit FloatType* asFloat() const;

  useit bool           isReference() const;
  useit ReferenceType* asReference() const;

  useit bool         isPointer() const;
  useit PointerType* asPointer() const;

  useit bool       isArray() const;
  useit ArrayType* asArray() const;

  useit bool       isTuple() const;
  useit TupleType* asTuple() const;

  useit bool          isFunction() const;
  useit FunctionType* asFunction() const;

  useit bool      isCoreType() const;
  useit CoreType* asCore() const;

  useit bool     isMix() const;
  useit MixType* asMix() const;

  useit bool        isChoice() const;
  useit ChoiceType* asChoice() const;

  useit bool             isStringSlice() const;
  useit StringSliceType* asStringSlice() const;

  useit bool        isFuture() const;
  useit FutureType* asFuture() const;

  useit bool       isMaybe() const;
  useit MaybeType* asMaybe() const;

  useit bool    isRegion() const;
  useit Region* asRegion() const;

  useit bool isVoid() const;

  useit bool       isTyped() const;
  useit TypedType* asTyped() const;

  useit bool   isCType() const;
  useit CType* asCType() const;

  useit bool        isResult() const;
  useit ResultType* asResult() const;

  useit virtual TypeKind typeKind() const = 0;
  useit virtual String   toString() const = 0;

  useit llvm::Type* getLLVMType() const { return llvmType; }
};

} // namespace qat::IR

#endif
