#include "./say_sentence.hpp"

namespace qat {
namespace AST {

/// TODO: Complete Implementation
IR::Value *SaySentence::emit(IR::Context *ctx) {
  // using qat::AST::NodeType;

  // std::string formatting;
  // std::vector<llvm::Value *> gen_exps;
  // for (std::size_t i = 0; i < expressions.size(); i++) {
  //   auto expr = expressions.at(i);
  //   auto gen_value = expr->emit(ctx);
  //   gen_exps.push_back(gen_value);
  //   switch (expr->nodeType()) {
  //   case NodeType::stringLiteral: {
  //     formatting += "%s";
  //     break;
  //   }
  //   case NodeType::integerLiteral: {
  //     formatting += "%d";
  //     break;
  //   }
  //   case NodeType::floatLiteral: {
  //     formatting += "%f";
  //     break;
  //   }
  //   case NodeType::sizeOfType: {
  //     formatting += "%d";
  //     break;
  //   }
  //   case NodeType::memberVariableExpression:
  //   case NodeType::entity: {
  //     auto vType = gen_value->getType();
  //     if (vType->isIntegerTy()) {
  //       formatting += "%d";
  //     } else if (vType->isFloatingPointTy()) {
  //       formatting += "%f";
  //     } else if (vType->isArrayTy() ? (llvm::dyn_cast<llvm::ArrayType>(vType)
  //                                          ->getElementType()
  //                                          ->isIntegerTy(8))
  //                                   : false) {
  //       formatting += "%s";
  //     } else if (vType->isPointerTy()) {
  //       formatting += "%p";
  //     } else if (vType->isStructTy()) {
  //       if (llvm::dyn_cast<llvm::StructType>(vType)->isLiteral()) {
  //         /// TODO:
  //       } else {
  //         // TODO - Handle string conversion
  //         // auto string_fn = ctx->mod->getFunction(
  //         //     vType->getStructName().str() + "'to'string");
  //         // if (string_fn) {
  //         //   formatting += "%s";
  //         //   std::vector<llvm::Value *> args;
  //         //   gen_exps.pop_back();
  //         //   args.push_back(gen_value);
  //         //   gen_exps.push_back(ctx->builder.CreateCall(
  //         //       string_fn->getFunctionType(), string_fn,
  //         //       llvm::ArrayRef<llvm::Value *>(args),
  //         //       llvm::Twine("Call to " + vType->getStructName().str() +
  //         //                   "'to'string"),
  //         //       nullptr));
  //         // } else {
  //         //   // TODO - Handle missing function
  //         // }
  //       }
  //     } else {
  //       formatting += "%n";
  //     }
  //     break;
  //   }
  //   case NodeType::memberFunctionCall:
  //   case NodeType::functionCall: {
  //     auto fn_call = llvm::dyn_cast<llvm::CallInst>(gen_value);
  //     auto return_ty = fn_call->getFunctionType()->getReturnType();
  //     if (return_ty->isIntegerTy()) {
  //       formatting += "%d";
  //     } else if (return_ty->isFloatingPointTy()) {
  //       formatting += "%f";
  //     } else if (return_ty->isArrayTy()
  //                    ? (llvm::dyn_cast<llvm::ArrayType>(return_ty)
  //                           ->getElementType()
  //                           ->isIntegerTy(8))
  //                    : false) {
  //       formatting += "%s";
  //     } else if (return_ty->isPointerTy()) {
  //       formatting += "%p";
  //     } else if (return_ty->isStructTy()) {
  //       // TODO - Handle string conversion
  //       // auto string_fn = ctx->get_function(
  //       //     return_ty->getStructName().str() + "'to'string");
  //       // if (string_fn) {
  //       //   formatting += "%s";
  //       //   std::vector<llvm::Value *> args;
  //       //   gen_exps.pop_back();
  //       //   args.push_back(gen_value);
  //       //   gen_exps.push_back(ctx->builder.CreateCall(
  //       //       string_fn->getFunctionType(), string_fn,
  //       //       llvm::ArrayRef<llvm::Value *>(args),
  //       //       llvm::Twine("Call to " + return_ty->getStructName().str() +
  //       //                   "'to'string"),
  //       //       nullptr));
  //       // } else {
  //       //   formatting += "%n";
  //       // }
  //     }
  //     break;
  //   }
  //   }
  // }
  // std::vector<llvm::Value *> printfArgs;
  // printfArgs.push_back(
  //     ctx->builder.CreateGlobalStringPtr(llvm::StringRef(formatting)));
  // for (std::size_t i = 0; i < gen_exps.size(); i++) {
  //   printfArgs.push_back(gen_exps.at(i));
  // }
  // // TODO - Handle printf
  // // auto printfFn = ctx->get_function("printf");
  // // if (printfFn == nullptr) {
  // //   ctx->throw_error(
  // //       "Cannot find `printf` function. Cannot compile `say` sentence",
  // //       file_placement);
  // // }
  // // return ctx->builder.CreateCall(
  // //     printfFn->getFunctionType(), printfFn,
  // //     llvm::ArrayRef<llvm::Value *>(printfArgs), "Call to printf",
  // nullptr);
}

void SaySentence::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file.addInclude("<iostream>");
    file += "std::cout << ";
    for (std::size_t i = 0; i < expressions.size(); i++) {
      expressions.at(i)->emitCPP(file, isHeader);
      if (i != (expressions.size() - 1)) {
        file += " << ";
      }
    }
    file += ";\n";
  }
}

backend::JSON SaySentence::toJSON() const {
  std::vector<backend::JSON> exps;
  for (auto exp : expressions) {
    exps.push_back(exp->toJSON());
  }
  return backend::JSON()
      ._("nodeType", "saySentence")
      ._("values", exps)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat