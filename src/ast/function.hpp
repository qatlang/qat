#ifndef QAT_AST_FUNCTION_HPP
#define QAT_AST_FUNCTION_HPP

#include "../IR/context.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./sentence.hpp"
#include "types/templated.hpp"
#include <iostream>
#include <string>

namespace qat::ast {

class FunctionPrototype : public Node {
private:
  friend class FunctionDefinition;
  String                name;
  bool                  isAsync;
  Vec<Argument *>       arguments;
  bool                  isVariadic;
  QatType              *returnType;
  String                callingConv;
  utils::VisibilityKind visibility;

  Vec<TemplatedType *>  templates;
  IR::TemplateFunction *templateFn;

  mutable llvm::GlobalValue::LinkageTypes linkageType;
  mutable IR::Function                   *function = nullptr;

public:
  FunctionPrototype(String _name, Vec<Argument *> _arguments, bool _isVariadic,
                    QatType *_returnType, bool _is_async,
                    llvm::GlobalValue::LinkageTypes _linkageType,
                    String _callingConv, utils::VisibilityKind _visibility,
                    const utils::FileRange &_fileRange,
                    Vec<TemplatedType *>    _templates = {});

  FunctionPrototype(const FunctionPrototype &ref);

  useit bool isTemplate() const;
  useit Vec<TemplatedType *> getTemplates() const;
  IR::Function              *createFunction(IR::Context *ctx) const;

  void  define(IR::Context *ctx) final;
  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::functionPrototype; }
};

class FunctionDefinition : public Node {
private:
  Vec<Sentence *> sentences;

public:
  FunctionDefinition(FunctionPrototype *_prototype, Vec<Sentence *> _sentences,
                     utils::FileRange _fileRange);

  FunctionPrototype *prototype;

  useit bool isTemplate() const;
  void       define(IR::Context *ctx) final;
  useit IR::Value *emit(IR::Context *ctx) final;
  useit NodeType nodeType() const final { return NodeType::functionDefinition; }
  useit nuo::Json toJson() const final;
};

} // namespace qat::ast

#endif
