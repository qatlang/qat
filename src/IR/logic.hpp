#ifndef QAT_IR_LOGIC_HPP
#define QAT_IR_LOGIC_HPP

#include "function.hpp"
#include "value.hpp"

namespace qat::IR {

class Logic {
public:
  static IR::Value        *handleCopyOrMove(IR::Value *value, IR::Context *ctx,
                                            const utils::FileRange &fileRange);
  static llvm::AllocaInst *newAlloca(IR::Function *fun, const String &name,
                                     llvm::Type *type);
  static String            getTemplateVariantName(String              mainName,
                                                  Vec<IR::QatType *> &types);
};

} // namespace qat::IR

#endif