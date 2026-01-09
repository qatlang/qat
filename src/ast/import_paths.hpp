#ifndef QAT_AST_IMPORT_PATHS_HPP
#define QAT_AST_IMPORT_PATHS_HPP

#include "./prerun/string_literal.hpp"

namespace qat::ast {

class ImportPaths final : public Node {
	bool                       isMember;
	Vec<StringLiteral*>        paths;
	Maybe<VisibilitySpec>      visibSpec;
	Vec<Maybe<StringLiteral*>> names;

  public:
	ImportPaths(bool _isMember, Vec<StringLiteral*> _paths, Vec<Maybe<StringLiteral*>> _names,
	            Maybe<VisibilitySpec> _visibSpec, FileRangePtr _fileRange)
	    : Node(_fileRange), isMember(_isMember), paths(_paths), visibSpec(_visibSpec), names(_names) {}

	useit static ImportPaths* create(bool _isMember, Vec<StringLiteral*> _paths, Vec<Maybe<StringLiteral*>> _names,
	                                 Maybe<VisibilitySpec> _visibSpec, FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(ImportPaths), _isMember, _paths, _names, _visibSpec, _fileRange);
	}

	void handle_filesystem_imports(ir::Mod* mod, ir::Ctx* irCtx) const final;

	useit NodeType nodeType() const final { return NodeType::IMPORT_PATHS; }
};

} // namespace qat::ast

#endif
