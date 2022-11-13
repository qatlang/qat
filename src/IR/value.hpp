#ifndef QAT_IR_VALUE_HPP
#define QAT_IR_VALUE_HPP

#include "../utils/json.hpp"
#include "llvm_helper.hpp"
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

namespace qat::IR {

class QatType;

enum class Nature { assignable, temporary, pure, expired };

class ConstantValue;
class Context;
class QatModule;
class Function;

class Value {
private:
  static Vec<IR::Value*> allValues;

protected:
  IR::QatType*  type;     // Type representation of the value
  Nature        nature;   // The nature of the value
  bool          variable; // Variability nature
  llvm::Value*  ll;       // LLVM value
  Maybe<String> localID;  // ID of the local in a function

public:
  Value(llvm::Value* _llValue, IR::QatType* _type, bool _isVariable, Nature kind);

  virtual ~Value() = default;

  useit QatType*             getType() const; // Type of the value
  useit virtual llvm::Value* getLLVM() const;
  useit bool                 isReference() const;
  useit bool                 isPointer() const;
  useit bool                 isVariable() const;
  useit virtual bool         isConstVal() const;
  useit ConstantValue*       asConst() const;
  useit bool                 isLocalToFn() const;
  useit String               getLocalID() const;
  void                       setLocalID(const String& locID);
  useit Nature               getNature() const;
  useit IR::Value*     createAlloca(llvm::IRBuilder<>& builder);
  useit bool           isImplicitPointer() const;
  void                 makeImplicitPointer(IR::Context* ctx, Maybe<String> name);
  void                 loadImplicitPointer(llvm::IRBuilder<>& builder);
  useit virtual Value* call(IR::Context* ctx, const Vec<llvm::Value*>& args, QatModule* mod);

  static void clearAll();
};

class ConstantValue : public Value {
public:
  ConstantValue(llvm::Constant* _llconst, IR::QatType* _type);

  useit llvm::Constant* getLLVM() const final;
  useit bool            isConstVal() const final;
};

} // namespace qat::IR

#endif