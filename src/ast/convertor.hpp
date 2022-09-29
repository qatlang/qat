#ifndef QAT_AST_CONVERTOR_HPP
#define QAT_AST_CONVERTOR_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "sentence.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class ConvertorPrototype : public Node {
  friend class ConvertorDefinition;
  friend class DefineCoreType;

private:
  String                argName;
  QatType              *candidateType;
  bool                  isMemberArgument;
  utils::VisibilityKind visibility;
  bool                  isFrom;

  mutable IR::CoreType       *coreType;
  mutable IR::MemberFunction *memberFn;

  void setCoreType(IR::CoreType *_coreType) const;

  ConvertorPrototype(bool _isFrom, String _argName, QatType *_candidateType,
                     bool _isMemberArg, utils::VisibilityKind _visibility,
                     const utils::FileRange &_fileRange);

public:
  static ConvertorPrototype *From(const String &_argName,
                                  QatType *_candidateType, bool _isMemberArg,
                                  utils::VisibilityKind   _visibility,
                                  const utils::FileRange &_fileRange);

  static ConvertorPrototype *To(QatType                *_candidateType,
                                utils::VisibilityKind   visib,
                                const utils::FileRange &fileRange);

  void  define(IR::Context *ctx) final;
  useit IR::Value *emit(IR::Context *ctx) final;
  useit Json       toJson() const final;
  useit NodeType nodeType() const final { return NodeType::convertorPrototype; }
};

class ConvertorDefinition : public Node {
  friend class DefineCoreType;

private:
  Vec<Sentence *>     sentences;
  ConvertorPrototype *prototype;

  void setCoreType(IR::CoreType *coreType) const;

public:
  ConvertorDefinition(ConvertorPrototype *_prototype,
                      Vec<Sentence *> _sentences, utils::FileRange _fileRange);

  void  define(IR::Context *ctx) final;
  useit IR::Value *emit(IR::Context *ctx) final;
  useit Json       toJson() const final;
  useit NodeType nodeType() const final { return NodeType::functionDefinition; }
};

} // namespace qat::ast

#endif
