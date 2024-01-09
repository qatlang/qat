#ifndef QAT_IR_LOGIC_HPP
#define QAT_IR_LOGIC_HPP

#include "./function.hpp"
#include "./generics.hpp"

namespace qat::IR {

class Logic {
public:
  useit static llvm::AllocaInst* newAlloca(IR::Function* fun, Maybe<String> name, llvm::Type* type);
  useit static String            getGenericVariantName(String mainName, Vec<IR::GenericToFill*>& types);
  useit static bool compareConstantStrings(llvm::Constant* lhsBuff, llvm::Constant* lhsCount, llvm::Constant* rhsBuff,
                                           llvm::Constant* rhsCount, llvm::LLVMContext& llCtx);

  static Pair<String, Vec<llvm::Value*>> formatValues(IR::Context* ctx, Vec<IR::Value*> values, Vec<FileRange> ranges,
                                                      FileRange fileRange);
  static void panicInFunction(IR::Function* fun, Vec<IR::Value*> values, Vec<FileRange> ranges, FileRange fileRange,
                              IR::Context* ctx);
};

} // namespace qat::IR

#endif