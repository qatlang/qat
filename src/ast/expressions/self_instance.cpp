#include "./self_instance.hpp"

namespace qat::ast {

IR::Value* SelfInstance::emit(IR::Context* ctx) {
  if (ctx->hasActiveFunction()) {
    if (ctx->getActiveFunction()->isMemberFunction()) {
      auto* mFn = (IR::MemberFunction*)ctx->getActiveFunction();
      if (mFn->getMemberFnType() == IR::MemberFnType::staticFn) {
        ctx->Error("This is a static method and hence the parent instance cannot be retrieved", fileRange);
      }
      if (mFn->getFirstBlock()->hasValue("''")) {
        auto selfVal = mFn->getFirstBlock()->getValue("''");
        return new IR::Value(selfVal->getLLVM(), selfVal->getType(),
                             mFn->getMemberFnType() == IR::MemberFnType::value_method, IR::Nature::temporary);
      } else {
        if (mFn->getType()->asFunction()->getArgumentTypeAt(0)->getName() == "''") {
          return new IR::Value(mFn->getLLVMFunction()->getArg(0),
                               mFn->getType()->asFunction()->getArgumentTypeAt(0)->getType(),
                               mFn->getMemberFnType() == IR::MemberFnType::value_method, IR::Nature::temporary);
        } else {
          ctx->Error("Cannot retrieve the parent instance of this method", fileRange);
        }
      }
    } else {
      ctx->Error("The current function is not a method of any type and hence the parent instance cannot be retrieved",
                 fileRange);
    }
  } else {
    ctx->Error("No active function could be found in the current scope", fileRange);
  }
}

Json SelfInstance::toJson() const { return Json()._("nodeType", "selfInstance")._("fileRange", fileRange); }

} // namespace qat::ast