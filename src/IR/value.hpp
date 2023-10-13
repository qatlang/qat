#ifndef QAT_IR_VALUE_HPP
#define QAT_IR_VALUE_HPP

#include "../IR/types/typed.hpp"
#include "../utils/json.hpp"
#include "llvm/IR/ConstantFolder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

namespace qat::IR {

class QatType;

enum class Nature { assignable, temporary, pure, expired };

class PrerunValue;
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
  useit PrerunValue*         asConst() const;
  useit bool                 isLLVMConstant() const;
  useit llvm::Constant* getLLVMConstant() const;
  useit bool            isLocalToFn() const;
  useit String          getLocalID() const;
  void                  setLocalID(const String& locID);
  useit Nature          getNature() const;
  useit bool            isImplicitPointer() const;
  void                  makeImplicitPointer(IR::Context* ctx, Maybe<String> name);
  void                  loadImplicitPointer(llvm::IRBuilder<>& builder);
  useit virtual Value*  call(IR::Context* ctx, const Vec<llvm::Value*>& args, QatModule* mod);

  static void clearAll();
};

class PrerunValue : public Value {
public:
  PrerunValue(llvm::Constant* _llconst, IR::QatType* _type);
  explicit PrerunValue(IR::TypedType* typed);

  ~PrerunValue() override = default;

  useit llvm::Constant* getLLVM() const final;

  bool isEqualTo(PrerunValue* other);

  useit bool isConstVal() const final;
};

} // namespace qat::IR

#endif