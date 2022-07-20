#ifndef QAT_IR_DEFINABLE_HPP
#define QAT_IR_DEFINABLE_HPP

#include "../backend/cpp.hpp"
#include "llvm_helper.hpp"
#include "llvm/IR/Value.h"
#include <nuo/json.hpp>

namespace qat::IR {

class Definable {
public:
  virtual void defineLLVM(llvmHelper helper) const {}
  virtual void defineCPP(cpp::File &file) const {}
  virtual nuo::Json toJson() const {}
};

} // namespace qat::IR

#endif