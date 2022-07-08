#include "./function_definition.hpp"
#include "../show.hpp"

namespace qat {
namespace AST {

FunctionDefinition::FunctionDefinition(FunctionPrototype *_prototype,
                                       std::vector<Sentence *> _sentences,
                                       utils::FilePlacement _filePlacement)
    : prototype(_prototype), sentences(_sentences), Node(_filePlacement) {}

IR::Value *FunctionDefinition::emit(IR::Context *ctx) {
  // SHOW("Before generating prototype")
  // auto function = llvm::dyn_cast<llvm::Function>(prototype->emit(ctx));
  // SHOW("Prototype generated")
  // auto entryBlock =
  //     llvm::BasicBlock::Create(ctx->llvmContext, "0", function, nullptr);
  // llvm::Value *lastSentence;
  // SHOW("BB created")
  // ctx->builder.SetInsertPoint(entryBlock);
  // std::vector<llvm::AllocaInst *> argAllocas;
  // SHOW("Creating arg allocas")
  // for (std::size_t i = 0; i < function->arg_size(); i++) {
  //   function->getArg(i)->setName(prototype->arguments.at(i)->getName());
  //   auto argAlloca =
  //       new llvm::AllocaInst(function->getArg(i)->getType(), 0,
  //                            prototype->arguments.at(i)->getName(),
  //                            entryBlock);
  //   lastSentence = argAlloca;
  //   argAllocas.push_back(argAlloca);
  //   bool variability = prototype->arguments.at(i)->getType()->isVariable();
  //   utils::PointerKind::set(
  //       ctx->llvmContext, argAlloca,
  //       utils::PointerKind::is_reference(function->getArg(i)));
  //   utils::Variability::set(ctx->llvmContext, argAlloca, variability);
  // }
  // SHOW("Arg allocas created")
  // SHOW("Arg stores")
  // for (std::size_t i = 0; i < function->arg_size(); i++) {
  //   lastSentence =
  //       ctx->builder.CreateStore(function->getArg(i), argAllocas.at(i),
  //       false);
  // }
  // SHOW("Arg stores done")
  // argAllocas.clear();
  // SHOW("Emitting sentences...")
  // if (sentences.size() > 0) {
  //   for (auto sentence : sentences) {
  //     lastSentence = sentence->emit(ctx);
  //   }
  // }
  // SHOW("Sentences emitted")
  // if (lastSentence) {
  //   // FIXME - Check logic for return
  //   if (!llvm::isa<llvm::ReturnInst>(lastSentence)) {
  //     if (prototype->returnType->emit(ctx)->getLLVMType()->isVoidTy()) {
  //       lastSentence = ctx->builder.CreateRetVoid();
  //     }
  //   }
  //   return lastSentence;
  // } else {
  //   return function;
  // }
}

void FunctionDefinition::emitCPP(backend::cpp::File &file,
                                 bool isHeader) const {
  prototype->emitCPP(file, isHeader);
  if (!isHeader) {
    file += " {\n";
    for (auto snt : sentences) {
      file += "  ";
      snt->emitCPP(file, isHeader);
    }
    file += "\n}\n";
  }
}

backend::JSON FunctionDefinition::toJSON() const {
  std::vector<backend::JSON> sntcs;
  for (auto sentence : sentences) {
    sntcs.push_back(sentence->toJSON());
  }
  return backend::JSON()
      ._("nodeType", "functionDefinition")
      ._("prototype", prototype->toJSON())
      ._("body", sntcs)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat