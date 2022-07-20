#ifndef QAT_AST_FUNCTION_PROTOTYPE_HPP
#define QAT_AST_FUNCTION_PROTOTYPE_HPP

#include "../IR/context.hpp"
#include "../utils/string_to_callingconv.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class FunctionPrototype : public Node {
protected:
  friend class FunctionDefinition;
  std::string name;
  bool isAsync;
  std::vector<Argument *> arguments;
  bool isVariadic;
  QatType *returnType;
  llvm::GlobalValue::LinkageTypes linkageType;
  std::string callingConv;
  utils::VisibilityInfo visibility;

public:
  FunctionPrototype(std::string _name, std::vector<Argument *> _arguments,
                    bool _isVariadic, QatType *_returnType, bool _is_async,
                    llvm::GlobalValue::LinkageTypes _linkageType,
                    std::string _callingConv, utils::VisibilityInfo _visibility,
                    utils::FileRange _fileRange)
      : name(_name), isAsync(_is_async), arguments(_arguments),
        isVariadic(_isVariadic), returnType(_returnType),
        linkageType(_linkageType), callingConv(_callingConv),
        visibility(_visibility), Node(_fileRange) {}

  FunctionPrototype(const FunctionPrototype &ref)
      : name(ref.name), isAsync(ref.isAsync), arguments(ref.arguments),
        isVariadic(ref.isVariadic), returnType(ref.returnType),
        linkageType(ref.linkageType), callingConv(ref.callingConv),
        visibility(ref.visibility), Node(ref.fileRange) {}

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::functionPrototype; }
};

} // namespace qat::ast

#endif
