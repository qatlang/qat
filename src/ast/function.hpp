#ifndef QAT_AST_FUNCTION_HPP
#define QAT_AST_FUNCTION_HPP

#include "../IR/context.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./sentence.hpp"
#include "types/generic_abstract.hpp"
#include <iostream>
#include <string>

namespace qat::ast {

class FunctionPrototype : public Node {
private:
  friend class FunctionDefinition;
  Identifier            name;
  bool                  isAsync;
  Vec<Argument*>        arguments;
  bool                  isVariadic;
  QatType*              returnType;
  String                callingConv;
  utils::VisibilityKind visibility;

  Vec<GenericAbstractType*> generics;
  IR::GenericFunction*      genericFn = nullptr;
  mutable Maybe<String>     variantName;

  mutable llvm::GlobalValue::LinkageTypes linkageType;
  mutable IR::Function*                   function = nullptr;

public:
  FunctionPrototype(Identifier _name, Vec<Argument*> _arguments, bool _isVariadic, QatType* _returnType, bool _is_async,
                    llvm::GlobalValue::LinkageTypes _linkageType, String _callingConv,
                    utils::VisibilityKind _visibility, const FileRange& _fileRange,
                    Vec<GenericAbstractType*> _generics = {});

  useit bool isGeneric() const;
  useit Vec<GenericAbstractType*> getGenerics() const;
  void                            setVariantName(const String& value) const;
  void                            unsetVariantName() const;
  IR::Function*                   createFunction(IR::Context* ctx) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::functionPrototype; }
  ~FunctionPrototype();
};

class FunctionDefinition : public Node {
private:
  Vec<Sentence*> sentences;

public:
  FunctionDefinition(FunctionPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange);

  FunctionPrototype* prototype;

  useit bool isGeneric() const;
  void       define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::functionDefinition; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif
