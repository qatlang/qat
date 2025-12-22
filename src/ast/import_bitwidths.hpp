#ifndef QAT_AST_IMPORT_BITWIDTHS_HPP
#define QAT_AST_IMPORT_BITWIDTHS_HPP

#include "./node.hpp"
#include "./types/qat_type.hpp"

namespace qat::ast {

class ImportBitwidths : public Node {
	Vec<ast::Type*> importedTypes;

  public:
	ImportBitwidths(Vec<ast::Type*> _importedTypes, FileRangePtr _fileRange)
	    : Node(_fileRange), importedTypes(_importedTypes) {}

	useit static ImportBitwidths* create(Vec<ast::Type*> importedTypes, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(ImportBitwidths), std::move(importedTypes), _fileRange);
	}

	void create_module(ir::Mod* mod, ir::Ctx* irCtx) const final;

	useit Json to_json() const final;

	useit NodeType nodeType() const final { return NodeType::IMPORT_BITWIDTHS; }
};

} // namespace qat::ast

#endif
