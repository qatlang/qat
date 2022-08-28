#ifndef QAT_AST_MEMBER_PROTOTYPE_HPP
#define QAT_AST_MEMBER_PROTOTYPE_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./node.hpp"
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
  IR::CoreType         *coreType;
  bool                  isStatic;

  MemberPrototype(bool isStatic, bool _isVariationFn, const String &_name,
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

  void setCoreType(IR::CoreType *_coreType);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::memberPrototype; }
};

} // namespace qat::ast

#endif
