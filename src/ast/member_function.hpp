#ifndef QAT_AST_MEMBER_FUNCTION_HPP
#define QAT_AST_MEMBER_FUNCTION_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class MemberPrototype : public Node {
  friend class MemberDefinition;

private:
  bool                  isVariationFn;
  String                name;
  bool                  isAsync;
  Vec<Argument *>       arguments;
  bool                  isVariadic;
  QatType              *returnType;
  utils::VisibilityKind kind;
  bool                  isStatic;

  mutable IR::CoreType       *coreType;
  mutable IR::MemberFunction *memberFn = nullptr;

  MemberPrototype(bool isStatic, bool _isVariationFn, String _name,
                  Vec<Argument *> _arguments, bool _isVariadic,
                  QatType *_returnType, bool _is_async,
                  utils::VisibilityKind   kind,
                  const utils::FileRange &_fileRange);

public:
  static MemberPrototype *Normal(bool _isVariationFn, const String &_name,
                                 const Vec<Argument *> &_arguments,
                                 bool _isVariadic, QatType *_returnType,
                                 bool _is_async, utils::VisibilityKind _kind,
                                 const utils::FileRange &_fileRange);

  static MemberPrototype *Static(const String          &_name,
                                 const Vec<Argument *> &_arguments,
                                 bool _isVariadic, QatType *_returnType,
                                 bool _is_async, utils::VisibilityKind _kind,
                                 const utils::FileRange &_fileRange);

  void setCoreType(IR::CoreType *_coreType) const;

  void  define(IR::Context *ctx) final;
  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::memberPrototype; }
};

class MemberDefinition : public Node {
private:
  Vec<Sentence *>  sentences;
  MemberPrototype *prototype;

public:
  MemberDefinition(MemberPrototype *_prototype, Vec<Sentence *> _sentences,
                   utils::FileRange _fileRange);

  void setCoreType(IR::CoreType *coreType) const;

  void  define(IR::Context *ctx) final;
  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::memberDefinition; }
};

} // namespace qat::ast

#endif
