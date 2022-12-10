#ifndef QAT_AST_MOD_INFO_HPP
#define QAT_AST_MOD_INFO_HPP

#include "./node.hpp"
#include "constants/string_literal.hpp"
#include "key_value.hpp"

namespace qat::parser {
class Parser;
}

namespace qat::ast {

class ModInfo : public Node {
  friend class parser::Parser;

private:
  Maybe<KeyValue<String>> foreignID;
  Maybe<StringLiteral*>   outputName;
  Vec<StringLiteral*>     linkLibs;

public:
  ModInfo(Maybe<StringLiteral*> _outputName, Maybe<KeyValue<String>> _foreignID, Vec<StringLiteral*> _linkLibs,
          FileRange _fileRange);

  void  createModule(IR::Context* ctx) const final;
  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }
  useit NodeType   nodeType() const final { return NodeType::modInfo; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif