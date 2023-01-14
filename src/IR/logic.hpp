#ifndef QAT_IR_LOGIC_HPP
#define QAT_IR_LOGIC_HPP

#include "function.hpp"

namespace qat::IR {

class Logic {
public:
  useit static llvm::AllocaInst* newAlloca(IR::Function* fun, const String& name, llvm::Type* type);
  useit static String            getGenericVariantName(String mainName, Vec<IR::QatType*>& types);
  useit static bool compareConstantStrings(llvm::Constant* lhsBuff, llvm::Constant* lhsCount, llvm::Constant* rhsBuff,
                                           llvm::Constant* rhsCount, llvm::LLVMContext& llCtx);
};

} // namespace qat::IR

#endif