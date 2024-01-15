#ifndef QAT_IR_VALUE_HPP
#define QAT_IR_VALUE_HPP

#include "../IR/types/typed.hpp"
#include "../utils/file_range.hpp"
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
  IR::QatType*  type;
  Nature        nature;
  bool          variable;
  llvm::Value*  ll;
  Maybe<String> localID;
  bool          isSelf = false;

public:
  Value(llvm::Value* _llValue, IR::QatType* _type, bool _isVariable, Nature kind);

  virtual ~Value() = default;

  useit virtual llvm::Value* getLLVM() const { return ll; }
  useit virtual bool         isPrerunValue() const { return false; }
  useit virtual Value* call(IR::Context* ctx, const Vec<llvm::Value*>& args, Maybe<String> localID, QatModule* mod);

  useit inline QatType*        getType() const { return type; }
  useit inline Maybe<String>   getLocalID() const { return localID; }
  useit inline llvm::Constant* getLLVMConstant() const { return llvm::cast<llvm::Constant>(ll); }
  useit inline PrerunValue*    asPrerun() const { return (PrerunValue*)this; }
  useit inline Nature          getNature() const { return nature; }

  useit inline bool isSelfValue() const { return isSelf; }
  useit inline bool isVariable() const { return variable; }
  useit inline bool isLLVMConstant() const { return llvm::dyn_cast<llvm::Constant>(ll); }
  useit inline bool isValue() const { return !isReference() && !isPrerunValue() && !isImplicitPointer(); }
  useit inline bool isLocalToFn() const { return localID.has_value(); }
  useit inline bool isReference() const { return type->isReference(); }
  useit inline bool isPointer() const { return type->isPointer(); }
  useit inline bool isImplicitPointer() const {
    return ll && (llvm::isa<llvm::AllocaInst>(ll) || llvm::isa<llvm::GlobalVariable>(ll));
  }

  inline void setSelf() { isSelf = true; }
  inline void setLocalID(const String& locID) { localID = locID; }

  void loadImplicitPointer(llvm::IRBuilder<>& builder);

  useit Value* makeLocal(IR::Context* ctx, Maybe<String> name, FileRange fileRange);

  static void clearAll();
};

class PrerunValue : public Value {
public:
  PrerunValue(llvm::Constant* _llconst, IR::QatType* _type);
  explicit PrerunValue(IR::TypedType* typed);

  ~PrerunValue() override = default;

  useit llvm::Constant* getLLVM() const final;

  bool isEqualTo(IR::Context* ctx, PrerunValue* other);

  useit bool isPrerunValue() const final;
};

} // namespace qat::IR

#endif