#ifndef QAT_IR_FUNCTION_HPP
#define QAT_IR_FUNCTION_HPP

#include "../utils/file_range.hpp"
#include "../utils/identifier.hpp"
#include "../utils/mentionable.hpp"
#include "../utils/qat_region.hpp"
#include "../utils/visibility.hpp"
#include "./argument.hpp"
#include "./generic_variant.hpp"
#include "./generics.hpp"
#include "./types/qat_type.hpp"
#include "./uniq.hpp"
#include "./value.hpp"
#include "link_names.hpp"
#include "meta_info.hpp"
#include "types/function.hpp"

#include <helpers/integers.hpp>
#include <helpers/maybe.hpp>
#include <helpers/string.hpp>
#include <helpers/vec.hpp>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>

#define DEFAULT_FUNCTION_LINKAGE llvm::GlobalValue::LinkageTypes::ExternalLinkage

namespace qat::ast {
class FunctionPrototype;
class GenericAbstractType;
class ConstructorDefinition;
class ConvertorDefinition;
class PrerunExpression;
} // namespace qat::ast

namespace qat::ir {

class Mod;
class FunctionCall;
class Ctx;
class Method;

enum class ExternFnType {
	C,
	CPP,
};

class LocalValue final : public Value, public Uniq, public Mentionable {
	String name;

  public:
	LocalValue(String name, ir::Type* type, bool is_variable, Function* fun, FileRangePtr fileRange);

	static LocalValue* get(String name, ir::Type* type, bool isVar, Function* fn, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(LocalValue), name, type, isVar, fn, fileRange);
	}

	~LocalValue() final = default;

	String            get_name() const;
	llvm::AllocaInst* get_alloca() const;
	FileRangePtr      get_file_range() const;
	ir::Value*        to_new_ir_value() const;
};

class UseValue final : public Value, public Uniq, public Mentionable {
	String name;

  public:
	UseValue(String _name, llvm::Value* _value, ir::Type* _type, FileRangePtr _fileRange)
	    : Value(_value, _type, false), name(_name) {
		associatedRange = _fileRange;
	}

	static UseValue* create(String name, llvm::Value* value, ir::Type* type, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(UseValue), std::move(name), value, type, fileRange);
	}

	String get_name() const { return name; }

	FileRangePtr get_range() const { return associatedRange.value(); }
};

class Block : public Uniq {
	friend Method;

  private:
	String            name;
	llvm::BasicBlock* bb;
	Vec<LocalValue*>  values;
	Vec<UseValue*>    usedValues;
	Block*            parent = nullptr;
	Vec<Block*>       children;
	Function*         fn;
	usize             index;
	Maybe<usize>      active;
	mutable Vec<u64>  movedValues;
	mutable bool      isGhost = false;
	mutable bool      hasGive = false;
	mutable bool      hasTodo = false;

	Block* prevBlock = nullptr;
	Block* nextBlock = nullptr;

  public:
	Maybe<FileRangePtr> fileRange;

	Block(Function* fn, Block* parent);

	Block(FileRangePtr fileRange, String astName, Function* fn, Block* parent);

	static Block* create(Function* fn, Block* parent) { return std::construct_at(OwnNormal(Block), fn, parent); }

	static Block* create(FileRangePtr fileRange, String astName, Function* fn, Block* parent) {
		return std::construct_at(OwnNormal(Block), fileRange, std::move(astName), fn, parent);
	}

	~Block() = default;

	String get_name() const { return name; }

	llvm::BasicBlock* get_bb() const { return bb; }

	bool has_previous_block() const { return prevBlock != nullptr; }

	Block* get_previous_block() const { return prevBlock; }

	bool has_next_block() const { return nextBlock != nullptr; }

	Block* get_next_block() const { return nextBlock; }

	bool has_parent() const { return parent != nullptr; }

	Block* get_parent() const { return parent; }

	Function* get_fn() const { return fn; }

	bool        has_value(const String& name) const;
	LocalValue* get_value(const String& name) const;

	LocalValue* new_local(const String& name, ir::Type* type, bool isVar, Ctx* ctx, FileRangePtr fileRange);

	UseValue* create_use_value(String name, llvm::Value* value, ir::Type* type, Ctx* ctx, FileRangePtr fileRange);

	bool has_used_value(String const& name) const {
		for (auto* it : usedValues) {
			if (it->get_name() == name) {
				return true;
			}
		}
		if (prevBlock && prevBlock->has_used_value(name)) {
			return true;
		}
		if (has_parent()) {
			return parent->has_used_value(name);
		}
		return false;
	}

	UseValue* get_used_value(String const& name) const {
		for (auto* it : usedValues) {
			if (it->get_name() == name) {
				return it;
			}
		}
		return nullptr;
	}

	bool is_moved(u64 locID) const;
	bool has_give_in_all_control_paths() const;

	bool has_todo() const { return hasTodo; }

	Block* get_active() {
		if (active) {
			return children.at(active.value())->get_active();
		} else {
			return this;
		}
	}

	Vec<LocalValue*>& get_locals() { return values; }

	Maybe<FileRangePtr> get_file_range() const {
		if (fileRange.has_value()) {
			return fileRange;
		}
		if (parent && parent->fileRange.has_value()) {
			return parent->fileRange;
		}
		return None;
	}

	void link_previous_block(Block* block) {
		prevBlock        = block;
		block->nextBlock = this;
	}

	void set_file_range(FileRangePtr _fileRange) { fileRange = _fileRange; }

	void set_has_give() const { hasGive = true; }

	void set_has_todo() const { hasTodo = true; }

	void add_moved_value(u64 locID) const { movedValues.push_back(locID); }

	void set_active(llvm::IRBuilder<>& builder);

	void collect_all_local_values_so_far(Vec<LocalValue*>& vals) const;

	void collect_locals_from(Vec<LocalValue*>& vals) const;

	void destroy_locals(ast::EmitCtx* ctx);
};

// Function represents a normal function in the language
class Function : public Value, public Uniq, public Mentionable {
	friend class Block;

  protected:
	Identifier            name;
	LinkNames             namingInfo;
	String                linkingName;
	Vec<GenericArgument*> generics;
	Mod*                  mod;
	Vec<Argument>         arguments;
	VisibilityInfo        visibilityInfo;
	Maybe<FileRangePtr>   fileRange;
	Maybe<Variadics>      variadics;
	bool                  isInline;
	Vec<Block*>           blocks;
	ir::LocalValue*       commonIndex = nullptr;
	Maybe<MetaInfo>       metaInfo;
	Ctx*                  ctx;

	mutable u64   localNameCounter = 0;
	mutable usize activeBlock      = 0;

  public:
	Function(Mod* mod, Identifier name, Maybe<LinkNames> namingInfo, Vec<GenericArgument*> generics, bool isInline,
	         ReturnType* returnType, Vec<Argument> args, Maybe<Variadics> variadics, Maybe<FileRangePtr> fileRange,
	         const VisibilityInfo& visibilityInfo, ir::Ctx* irCtx, bool isMemberFn = false,
	         Maybe<llvm::GlobalValue::LinkageTypes> linkage = None, Maybe<MetaInfo> metaInfo = None);

	static Function* Create(Mod* mod, Identifier name, Maybe<LinkNames> namingInfo, Vec<GenericArgument*> generics,
	                        bool isInline, ReturnType* returnType, Vec<Argument> args, Maybe<Variadics> variadics,
	                        Maybe<FileRangePtr> fileRange, VisibilityInfo const& visibilityInfo, ir::Ctx* irCtx,
	                        Maybe<llvm::GlobalValue::LinkageTypes> linkage = None, Maybe<MetaInfo> metaInfo = None);

	Value* call(Ctx* irCtx, const Vec<llvm::Value*>& args, Maybe<u64> localID, Mod* mod) override;

	virtual bool is_method() const { return false; }

	Method* as_method() { return reinterpret_cast<Method*>(this); }

	bool has_variadic_args() const { return variadics.has_value(); }

	Variadics get_variadics() const { return variadics.value(); }

	Identifier arg_name_at(u32 index) const { return arguments[index].get_name(); }

	virtual Identifier get_name() const { return name; }

	virtual String get_full_name() const;

	bool is_accessible(const AccessInfo& req_info) const { return visibilityInfo.is_accessible(req_info); }

	VisibilityInfo const& get_visibility() const { return visibilityInfo; }

	ir::Mod* get_module() const { return mod; }

	llvm::Function* get_llvm_function() { return llvm::cast<llvm::Function>(ll); }

	Block* get_block() const { return blocks.at(activeBlock)->get_active(); }

	Block* get_first_block() const { return blocks[0]; }

	usize get_block_count() const { return blocks.size(); }

	bool is_inline() const { return isInline; }

	LocalValue* get_str_comparison_index(ir::Ctx* irCtx);

	bool is_generic() const { return !generics.empty(); }

	bool has_generic_parameter(const String& name) const {
		for (auto* gen : generics) {
			if (gen->get_name().value == name) {
				return true;
			}
		}
		return false;
	}

	GenericArgument* get_generic_parameter(const String& name) const {
		for (auto* gen : generics) {
			if (gen->get_name().value == name) {
				return gen;
			}
		}
		return nullptr;
	}

	bool has_definition_range() const { return fileRange.has_value(); }

	FileRangePtr get_definition_range() const { return fileRange.value(); }

	String get_random_alloca_name() const {
		localNameCounter++;
		return std::to_string(localNameCounter) + "_new";
	}

	void set_active_block(usize index) const { activeBlock = index; }

	~Function() override;
};

class GenericFunction : public Uniq, public Mentionable {
	Identifier                     name;
	Vec<ast::GenericAbstractType*> generics;
	ast::FunctionPrototype*        functionDefinition;
	ast::PrerunExpression*         constraint;
	Mod*                           parent;
	VisibilityInfo                 visibInfo;

	mutable Vec<GenericVariant<Function>> variants;

  public:
	GenericFunction(Identifier name, Vec<ast::GenericAbstractType*> _generics, ast::PrerunExpression* constraint,
	                ast::FunctionPrototype* functionDef, Mod* parent, const VisibilityInfo& _visibInfo);

	static GenericFunction* create(Identifier name, Vec<ast::GenericAbstractType*> _generics,
	                               ast::PrerunExpression* constraint, ast::FunctionPrototype* functionDef, Mod* parent,
	                               const VisibilityInfo& _visibInfo) {
		return std::construct_at(OwnNormal(GenericFunction), std::move(name), std::move(_generics), constraint,
		                         functionDef, parent, _visibInfo);
	}

	~GenericFunction() {
		for (auto& it : variants) {
			it.clear_fill_types();
		}
	}

	Identifier                get_name() const;
	usize                     getTypeCount() const;
	usize                     getVariantCount() const;
	Mod*                      get_module() const;
	ast::GenericAbstractType* getGenericAt(usize index) const;
	VisibilityInfo            get_visibility() const;
	Function*                 fill_generics(Vec<ir::GenericToFill*> _types, Ctx* irCtx, FileRangePtr fileRange);
	bool                      all_generics_have_default() const;
};

void function_return_handler(ir::Ctx* irCtx, ir::Function* fun, FileRangePtr fileRange);
void destructor_caller(ir::Ctx* irCtx, ir::Function* fun);
void method_handler(ir::Ctx* irCtx, ir::Function* fun);
void destroy_locals_from(ir::Ctx* irCtx, ir::Block* block);

} // namespace qat::ir

#endif
