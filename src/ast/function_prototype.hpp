#ifndef QAT_AST_FUNCTION_PROTOTYPE_HPP
#define QAT_AST_FUNCTION_PROTOTYPE_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class FunctionPrototype : public Node {
private:
  friend class FunctionDefinition;
  String                          name;
  bool                            isAsync;
  Vec<Argument *>                 arguments;
  bool                            isVariadic;
  QatType                        *returnType;
  llvm::GlobalValue::LinkageTypes linkageType;
  String                          callingConv;
  utils::VisibilityKind           visibility;

public:
  FunctionPrototype(const String &_name, Vec<Argument *> _arguments,
                    bool _isVariadic, QatType *_returnType, bool _is_async,
                    llvm::GlobalValue::LinkageTypes _linkageType,
                    const String                   &_callingConv,
                    utils::VisibilityKind           _visibility,
                    const utils::FileRange         &_fileRange);

  FunctionPrototype(const FunctionPrototype &ref);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::functionPrototype; }
};

} // namespace qat::ast

#endif
