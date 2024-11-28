#ifndef QAT_IR_INTERNAL_HPP
#define QAT_IR_INTERNAL_HPP

namespace qat::ir {

class Ctx;
class FunctionType;

class Internal {
public:
  static FunctionType* printf_signature(Ctx* irCtx);
  static FunctionType* malloc_signature(Ctx* irCtx);
  static FunctionType* realloc_signature(Ctx* irCtx);
  static FunctionType* free_signature(Ctx* irCtx);

  // static FunctionType* pthread_create_signature(Ctx* irCtx);
  // static FunctionType* pthread_join_signature(Ctx* irCtx);
  // static FunctionType* pthread_exit_signature(Ctx* irCtx);
  // static FunctionType* pthread_attr_init_signature(Ctx* irCtx);
  // static FunctionType* panic_handler_signature(Ctx* irCtx);
};

} // namespace qat::ir

#endif
