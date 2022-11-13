#ifndef QAT_AST_MOD_INFO_HPP
#define QAT_AST_MOD_INFO_HPP

#include "./node.hpp"
#include "key_value.hpp"

namespace qat::parser {
class Parser;
}

namespace qat::ast {

class ModInfo : public Node {
  friend class parser::Parser;

private:
  Maybe<KeyValue<String>>               foreignID;
  Maybe<Pair<String, utils::FileRange>> outputName;
  Vec<Pair<String, utils::FileRange>>   linkLibs;

public:
  ModInfo(Maybe<Pair<String, utils::FileRange>> _outputName, Maybe<KeyValue<String>> _foreignID,
          Vec<Pair<String, utils::FileRange>> _linkLibs, utils::FileRange _fileRange);

  void  createModule(IR::Context* ctx) const final;
  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }
  useit NodeType   nodeType() const final { return NodeType::modInfo; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif