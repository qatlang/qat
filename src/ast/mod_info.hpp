#ifndef QAT_AST_MOD_INFO_HPP
#define QAT_AST_MOD_INFO_HPP

#include "./node.hpp"
#include "key_value.hpp"
#include "meta_info.hpp"
#include "prerun/string_literal.hpp"

namespace qat::parser {
class Parser;
}

namespace qat::ast {

class ModInfo : public Node {
  friend class parser::Parser;

private:
  ast::MetaInfo metaInfo;

public:
  ModInfo(MetaInfo metaInfo, FileRange _fileRange);

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }
  useit NodeType   nodeType() const final { return NodeType::modInfo; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif