#ifndef QAT_AST_DESTRUCTOR_HPP
#define QAT_AST_DESTRUCTOR_HPP

#include "../IR/context.hpp"
#include "../IR/types/core_type.hpp"
#include "./argument.hpp"
#include "./sentence.hpp"
#include "./types/qat_type.hpp"
#include "llvm/IR/GlobalValue.h"
#include <string>

namespace qat::ast {

class DestructorDefinition : public Node {
private:
  FileRange      nameRange;
  Vec<Sentence*> sentences;

  mutable IR::MemberFunction* memberFn = nullptr;
  mutable IR::CoreType*       coreType = nullptr;

public:
  DestructorDefinition(FileRange nameRange, Vec<Sentence*> _sentences, FileRange _fileRange);

  void setCoreType(IR::CoreType* coreType) const;

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::memberDefinition; }
};

} // namespace qat::ast

#endif
