#include "./logic.hpp"
#include "context.hpp"
#include "function.hpp"
#include "types/core_type.hpp"
#include "types/reference.hpp"
#include "value.hpp"

namespace qat::IR {

String Logic::getTemplateVariantName(String mainName, Vec<IR::QatType*>& types) {
  String result(std::move(mainName));
  result += "'<";
  for (usize i = 0; i < types.size(); i++) {
    result += types.at(i)->toString();
    if (i != (types.size() - 1)) {
      result += ", ";
    }
  }
  result += ">";
  return result;
}

llvm::AllocaInst* Logic::newAlloca(IR::Function* fun, const String& name, llvm::Type* type) {
  llvm::AllocaInst* result = nullptr;
  llvm::Function*   func   = fun->isAsyncFunction() ? fun->getAsyncSubFunction() : fun->getLLVMFunction();
  if (func->getEntryBlock().getInstList().empty()) {
    result = new llvm::AllocaInst(type, 0U, name, &func->getEntryBlock());
  } else {
    llvm::Instruction* inst = nullptr;
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto instr = func->getEntryBlock().getInstList().begin(); instr != func->getEntryBlock().getInstList().end();
         instr++) {
      if (llvm::isa<llvm::AllocaInst>(&*instr)) {
        continue;
      } else {
        inst = &*instr;
        break;
      }
    }
    if (inst) {
      result = new llvm::AllocaInst(type, 0U, name, inst);
    } else {
      result = new llvm::AllocaInst(type, 0U, name, &func->getEntryBlock());
    }
  }
  return result;
}

} // namespace qat::IR