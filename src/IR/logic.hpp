#ifndef QAT_IR_LOGIC_HPP
#define QAT_IR_LOGIC_HPP

#include "./function.hpp"
#include "./generics.hpp"

namespace qat::ast {

struct EmitCtx;

}

namespace qat::ir {

class Logic {
  public:
	static llvm::AllocaInst* newAlloca(ir::Function* fun, Maybe<String> name, llvm::Type* type);

	static String get_generic_variant_name(String mainName, Vec<ir::GenericToFill*>& types);

	static bool compare_prerun_text(llvm::Constant* lhsBuff, llvm::Constant* lhsCount, llvm::Constant* rhsBuff,
	                                llvm::Constant* rhsCount, llvm::LLVMContext& llCtx);

	static ir::Value* compare_text(bool isEquality, ir::Value* lhs, ir::Value* rhs, FileRangePtr lhsRange,
	                               FileRangePtr rhsRange, FileRangePtr fileRange, ast::EmitCtx* ctx);

	static Pair<String, Vec<llvm::Value*>> format_values(ast::EmitCtx* ctx, Vec<ir::Value*> values,
	                                                     Vec<FileRangePtr> ranges, FileRangePtr fileRange);

	static void panic_in_function(ir::Function* fun, Vec<ir::Value*> values, Vec<FileRangePtr> ranges,
	                              FileRangePtr fileRange, ast::EmitCtx* ctx);

	static void exit_thread(ir::Function* fun, ast::EmitCtx* ctx, FileRangePtr rangeVal);
	static void exit_program(ir::Function* fun, ast::EmitCtx* ctx, FileRangePtr rangeVal);

	static ir::Value* int_to_std_string(bool isSigned, ast::EmitCtx* ctx, ir::Value* value, FileRangePtr fileRange);

	/// NOTE - This function should ideally control copy & move semantics behaviour for the entire language
	static ir::Value* handle_pass_semantics(ast::EmitCtx* ctx, ir::Type* expectedType, ir::Value* value,
	                                        FileRangePtr valueRange, bool restricLocalRefs = false);

	static Maybe<bool> is_atomic_qualified_type(llvm::Type* type, Ctx* ctx);
};

} // namespace qat::ir

#endif
