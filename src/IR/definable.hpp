#ifndef QAT_IR_DEFINABLE_HPP
#define QAT_IR_DEFINABLE_HPP

#include "../backend/cpp.hpp"
#include "../backend/json.hpp"
#include "llvm_helper.hpp"
#include "llvm/IR/Value.h"

namespace qat::IR {

class Definable {
public:
  virtual void defineLLVM(llvmHelper helper) const {}
  virtual void defineCPP(backend::cpp::File &file) const {}
  virtual backend::JSON toJSON() const {}
};

} // namespace qat::IR

#endif