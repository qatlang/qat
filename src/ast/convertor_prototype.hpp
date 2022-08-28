#ifndef QAT_AST_CONVERTOR_PROTOTYPE_HPP
#define QAT_AST_CONVERTOR_PROTOTYPE_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class ConvertorPrototype : public Node {
  friend class ConvertorDefinition;
  friend class DefineCoreType;

private:
  IR::CoreType         *coreType;
  String                argName;
  QatType              *candidateType;
  utils::VisibilityKind visibility;
  bool                  isFrom;

  ConvertorPrototype(bool _isFrom, String _argName, QatType *_candidateType,
                     utils::VisibilityKind   _visibility,
                     const utils::FileRange &_fileRange);

public:
  static ConvertorPrototype *From(const String           &_argName,
                                  QatType                *_candidateType,
                                  utils::VisibilityKind   _visibility,
                                  const utils::FileRange &_fileRange);

  static ConvertorPrototype *To(QatType                *_candidateType,
                                utils::VisibilityKind   visib,
                                const utils::FileRange &fileRange);

  void setCoreType(IR::CoreType *_coreType);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType nodeType() const final { return NodeType::convertorPrototype; }
};

} // namespace qat::ast

#endif
