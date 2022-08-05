#ifndef QAT_IR_TYPES_TUPLE_HPP
#define QAT_IR_TYPES_TUPLE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

#include <vector>

namespace qat::IR {

/**
 *  Tuples are product types. It is a defined fixed-length sequence of
 * other types
 *
 */
class TupleType : public QatType {
private:
  Vec<QatType *> subTypes;
  bool           isPacked;

  TupleType(Vec<QatType *> _types, bool _isPacked, llvm::LLVMContext &ctx);

public:
  useit static TupleType *get(Vec<QatType *> types, bool isPacked,
                              llvm::LLVMContext &ctx);

  useit Vec<QatType *> getSubTypes() const;
  useit QatType       *getSubtypeAt(u64 index);
  useit u64            getSubTypeCount() const;
  useit bool           isPackedTuple() const;
  useit TypeKind       typeKind() const override;
  useit String         toString() const override;
  useit nuo::Json toJson() const override;
};

} // namespace qat::IR

#endif