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
  useit static llvm::AllocaInst* newAlloca(ir::Function* fun, Maybe<String> name, llvm::Type* type);
  useit static String            get_generic_variant_name(String mainName, Vec<ir::GenericToFill*>& types);
  useit static bool compareConstantStrings(llvm::Constant* lhsBuff, llvm::Constant* lhsCount, llvm::Constant* rhsBuff,
                                           llvm::Constant* rhsCount, llvm::LLVMContext& llCtx);

  static Pair<String, Vec<llvm::Value*>> format_values(ast::EmitCtx* ctx, Vec<ir::Value*> values, Vec<FileRange> ranges,
                                                       FileRange fileRange);

  static void panic_in_function(ir::Function* fun, Vec<ir::Value*> values, Vec<FileRange> ranges, FileRange fileRange,
                                ast::EmitCtx* ctx);

  static void exit_thread(ir::Function* fun, ast::EmitCtx* ctx);

  static ir::Value* int_to_std_string(bool isSigned, ast::EmitCtx* ctx, ir::Value* value, FileRange fileRange);
};

} // namespace qat::ir

#endif