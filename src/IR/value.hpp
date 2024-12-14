#ifndef QAT_IR_VALUE_HPP
#define QAT_IR_VALUE_HPP

#include "../IR/types/typed.hpp"
#include "../utils/file_range.hpp"
#include "../utils/qat_region.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

namespace qat::ast {

struct EmitCtx;

}

namespace qat::ir {

class Type;
class PrerunLocal;
class PrerunValue;
class Ctx;
class Mod;
class Function;
class PrerunFunction;

class Value {
  protected:
	ir::Type*        type;
	bool             variable;
	llvm::Value*     ll;
	Maybe<u64>       localID;
	bool             isSelf = false;
	Maybe<FileRange> associatedRange;

  public:
	static Vec<ir::Value*> allValues;

	Value(llvm::Value* _llValue, ir::Type* _type, bool _isVariable);

	useit static Value* get(llvm::Value* ll, ir::Type* type, bool isVar) {
		return std::construct_at(OwnNormal(Value), ll, type, isVar);
	}

	virtual ~Value() = default;

	useit virtual llvm::Value* get_llvm() const { return ll; }

	useit virtual bool is_prerun_value() const { return false; }

	useit virtual Value* call(ir::Ctx* irCtx, const Vec<llvm::Value*>& args, Maybe<u64> localID, Mod* mod);

	useit Type* get_ir_type() const { return type; }

	useit Maybe<u64> get_local_id() const { return localID; }

	useit llvm::Constant* get_llvm_constant() const { return llvm::cast<llvm::Constant>(ll); }

	useit PrerunValue* as_prerun() const { return (PrerunValue*)this; }

	useit bool is_self_value() const { return isSelf; }

	useit bool is_variable() const { return variable; }

	useit bool is_llvm_constant() const { return llvm::dyn_cast<llvm::Constant>(ll); }

	useit bool is_value() const { return !is_reference() && !is_prerun_value() && !is_ghost_reference(); }

	useit bool is_local_value() const { return localID.has_value(); }

	useit bool is_reference() const { return type->is_reference(); }

	useit bool is_mark() const { return type->is_mark(); }

	useit bool is_ghost_reference() const {
		return ll && (((llvm::isa<llvm::AllocaInst>(ll) &&
		                llvm::cast<llvm::AllocaInst>(ll)->getAllocatedType() == get_ir_type()->get_llvm_type()) ||
		               (llvm::isa<llvm::GlobalVariable>(ll) &&
		                llvm::cast<llvm::GlobalVariable>(ll)->getValueType() == get_ir_type()->get_llvm_type())) &&
		              !is_prerun_value());
	}

	useit bool is_prerun_function() const;

	useit ir::PrerunFunction* as_prerun_function() const { return (ir::PrerunFunction*)ll; }

	useit ir::Value* with_range(FileRange rangeVal) {
		associatedRange = rangeVal;
		return this;
	}

	useit bool has_associated_range() const { return associatedRange.has_value(); }

	useit FileRange get_associated_range() const { return associatedRange.value(); }

	void set_self() { isSelf = true; }

	void set_local_id(const u64& locID) { localID = locID; }

	void load_ghost_reference(llvm::IRBuilder<>& builder) {
		if (is_ghost_reference()) {
			ll = builder.CreateLoad(get_ir_type()->get_llvm_type(), ll);
		}
	}

	useit Value* make_local(ast::EmitCtx* ctx, Maybe<String> name, FileRange fileRange);

	static void replace_uses_for_all() {
		for (auto itVal : allValues) {
			if (itVal && (not itVal->is_prerun_function()) && itVal->get_llvm()) {
				itVal->get_llvm()->replaceAllUsesWith(llvm::UndefValue::get(itVal->get_llvm()->getType()));
			}
		}
	}
	static void clear_all();
};

class PrerunValue : public Value {
  public:
	PrerunValue(llvm::Constant* _llConst, ir::Type* _type) : Value(_llConst, _type, false) {}

	explicit PrerunValue(ir::TypedType* _typed) : Value(nullptr, _typed, false) {}

	useit static PrerunValue* get(llvm::Constant* ll, ir::Type* type) {
		return std::construct_at(OwnNormal(PrerunValue), ll, type);
	}

	useit static PrerunValue* get_typed_prerun(ir::TypedType* typed) {
		return std::construct_at(OwnNormal(PrerunValue), typed);
	}

	~PrerunValue() override = default;

	useit llvm::Constant* get_llvm() const final { return (llvm::Constant*)(ll); }

	virtual bool is_prerun_local() const { return false; };

	useit PrerunLocal* as_prerun_local() { return reinterpret_cast<PrerunLocal*>(this); }

	bool is_equal_to(ir::Ctx* irCtx, PrerunValue* other);

	useit bool is_prerun_value() const final { return true; }
};

} // namespace qat::ir

#endif
