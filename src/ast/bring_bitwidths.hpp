#ifndef QAT_AST_BRING_BITWIDTHS_HPP
#define QAT_AST_BRING_BITWIDTHS_HPP

#include "node.hpp"
#include "types/qat_type.hpp"

namespace qat::ast {

class BringBitwidths : public Node {
	Vec<ast::Type*> broughtTypes;

  public:
	BringBitwidths(Vec<ast::Type*> _broughtTypes, FileRange _fileRange)
		: Node(_fileRange), broughtTypes(_broughtTypes) {}

	useit static BringBitwidths* create(Vec<ast::Type*> _broughtTypes, FileRange _fileRange) {
		return std::construct_at(OwnNormal(BringBitwidths), _broughtTypes, _fileRange);
	}

	void		   create_module(ir::Mod* mod, ir::Ctx* irCtx) const final;
	useit Json	   to_json() const final;
	useit NodeType nodeType() const final { return NodeType::BRING_BITWIDTHS; }
};

} // namespace qat::ast

#endif
