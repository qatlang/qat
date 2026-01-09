#ifndef QAT_IR_VALUE_HPP
#define QAT_IR_VALUE_HPP

#include "../utils/file_range.hpp"
#include "../utils/qat_region.hpp"
#include "./types/address_space.hpp"
#include "./types/qat_type.hpp"

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
	ir::Type*           type;
	bool                variable;
	llvm::Value*        ll;
	Maybe<u64>          localID;
	bool                isSelf = false;
	Maybe<FileRangePtr> associatedRange;
	bool                isConfirmedRef = false;

  public:
	static Vec<ir::Value*> allValues;

	Value(llvm::Value* _llValue, ir::Type* _type, bool _isVariable);

	static Value* get(llvm::Value* ll, ir::Type* type, bool isVar);

	virtual ~Value() = default;

	virtual llvm::Value* get_llvm() const { return ll; }

	virtual bool is_prerun_value() const { return false; }

	virtual Value* call(ir::Ctx* irCtx, const Vec<llvm::Value*>& args, Maybe<u64> localID, Mod* mod);

	Type* get_ir_type() const { return type; }

	Maybe<u64> get_local_id() const { return localID; }

	llvm::Constant* get_llvm_constant() const { return llvm::cast<llvm::Constant>(ll); }

	PrerunValue* as_prerun() const { return (PrerunValue*)this; }

	bool is_self_value() const { return isSelf; }

	bool has_variability() const { return variable; }

	bool is_llvm_constant() const { return llvm::dyn_cast<llvm::Constant>(ll); }

	bool is_value() const { return not is_ref() && not is_prerun_value() && not is_ghost_ref(); }

	bool is_local_value() const { return localID.has_value(); }

	void set_confirmed_ref() { isConfirmedRef = true; }

	// Not useful for prerun values
	ir::Type* get_pass_type() const;

	bool should_be_ref() const { return isConfirmedRef; }

	bool is_ref() const { return type->is_ref(); }

	bool is_ptr() const { return type->is_ptr(); }

	Maybe<AddressSpace> extract_address_space(ir::Ctx* irCtx) const;

	bool is_ghost_ref() const {
		return ll && (((llvm::isa<llvm::AllocaInst>(ll) &&
		                llvm::cast<llvm::AllocaInst>(ll)->getAllocatedType() == get_ir_type()->get_llvm_type()) ||
		               (llvm::isa<llvm::GlobalVariable>(ll) &&
		                llvm::cast<llvm::GlobalVariable>(ll)->getValueType() == get_ir_type()->get_llvm_type())) &&
		              not is_prerun_value());
	}

	bool is_prerun_function() const;

	ir::PrerunFunction* as_prerun_function() const { return (ir::PrerunFunction*)ll; }

	ir::Value* with_range(FileRangePtr rangeVal) {
		associatedRange = rangeVal;
		return this;
	}

	bool has_associated_range() const { return associatedRange.has_value(); }

	FileRangePtr get_associated_range() const { return associatedRange.value(); }

	void set_self() { isSelf = true; }

	void set_local_id(const u64& locID) { localID = locID; }

	void load_ghost_ref(llvm::IRBuilder<>& builder) {
		if (is_ghost_ref()) {
			ll = builder.CreateLoad(get_ir_type()->get_llvm_type(), ll);
		}
	}

	Value* make_local(ast::EmitCtx* ctx, Maybe<String> name, FileRangePtr fileRange);

	static void clear_all();
};

class PrerunValue : public Value {
  public:
	PrerunValue(llvm::Constant* _llConst, ir::Type* _type) : Value(_llConst, _type, true) {}

	static PrerunValue* get(llvm::Constant* ll, ir::Type* type) {
		return std::construct_at(OwnNormal(PrerunValue), ll, type);
	}

	~PrerunValue() override = default;

	llvm::Constant* get_llvm() const final { return (llvm::Constant*)(ll); }

	virtual bool is_prerun_local() const { return false; };

	PrerunLocal* as_prerun_local() { return reinterpret_cast<PrerunLocal*>(this); }

	bool is_equal_to(ir::Ctx* irCtx, PrerunValue* other);

	bool is_prerun_value() const final { return true; }
};

} // namespace qat::ir

#endif
