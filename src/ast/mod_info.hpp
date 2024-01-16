#ifndef QAT_AST_MOD_INFO_HPP
#define QAT_AST_MOD_INFO_HPP

#include "./node.hpp"
#include "meta_info.hpp"

namespace qat::parser {
class Parser;
}

namespace qat::ast {

class ModInfo : public Node {
  friend class parser::Parser;

private:
  ast::MetaInfo metaInfo;

public:
  ModInfo(MetaInfo _metaInfo, FileRange _fileRange) : Node(_fileRange), metaInfo(_metaInfo) {}

  useit static inline ModInfo* create(MetaInfo _metaInfo, FileRange _fileRange) {
    return std::construct_at(OwnNormal(ModInfo), _metaInfo, _fileRange);
  }

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }
  useit NodeType   nodeType() const final { return NodeType::MODULE_INFO; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif