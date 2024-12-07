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

	useit static ModInfo* create(MetaInfo _metaInfo, FileRange _fileRange) {
		return std::construct_at(OwnNormal(ModInfo), _metaInfo, _fileRange);
	}

	void create_module(ir::Mod* mod, ir::Ctx* irCtx) const final;

	useit NodeType nodeType() const final { return NodeType::MODULE_INFO; }
	useit Json	   to_json() const final;
};

} // namespace qat::ast

#endif
