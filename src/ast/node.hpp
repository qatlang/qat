#ifndef QAT_AST_NODE_HPP
#define QAT_AST_NODE_HPP

#include "../IR/context.hpp"
#include "../IR/types/choice.hpp"
#include "../IR/types/mix.hpp"
#include "../IR/types/text.hpp"
#include "../utils/file_range.hpp"
#include "./node_type.hpp"

#include <helpers/pair.hpp>
#include <helpers/string.hpp>
#include <helpers/vec.hpp>

namespace qat::ast {

struct VisibilitySpec {
	VisibilityKind kind;
	FileRangePtr   range;

	String to_string() const {
		switch (kind) {
			case VisibilityKind::type:
				return "pub:type";
			case VisibilityKind::pub:
				return "pub";
			case VisibilityKind::lib:
				return "pub:lib";
			case VisibilityKind::file:
				return "pub:file";
			case VisibilityKind::folder:
				return "pub:folder";
			case VisibilityKind::parent:
				return "pub:parent";
			case VisibilityKind::skill:
				return "pub:skill";
		}
	}
};

class Commentable {
  public:
	Maybe<Pair<String, FileRangePtr>> commentValue;

	bool hasCommentValue() const { return commentValue.has_value(); }
};

#define COMMENTABLE_FUNCTIONS                                                                                          \
	bool         isCommentable() const final { return true; }                                                          \
	Commentable* asCommentable() final { return (Commentable*)this; }

#define UPDATE_DEPS(x)               x->update_dependencies(phase, ir::DependType::complete, ent, ctx)
#define UPDATE_DEPS_CUSTOM(x, depTy) x->update_dependencies(phase, ir::DependType::depTy, ent, ctx)

// Node is the base class for all AST members of the language, and it
// requires a FileRange instance that indicates its position in the
// corresponding file
class Node {
  private:
	static Vec<Node*> allNodes;

  public:
	FileRangePtr fileRange;

	explicit Node(FileRangePtr _fileRange);
	virtual ~Node() = default;

	virtual bool isCommentable() const { return false; }

	virtual Commentable* asCommentable() { return nullptr; }

	virtual bool isPrerunNode() const { return false; }

	virtual void create_module(ir::Mod*, ir::Ctx*) const {}

	virtual void handle_filesystem_imports(ir::Mod*, ir::Ctx*) const {}

	virtual bool is_entity() const { return false; }

	virtual NodeType nodeType() const = 0;

	static void clear_all();
};

class IsEntity : public Node {
  public:
	ir::EntityState* entityState = nullptr;

	IsEntity(FileRangePtr _fileRange) : Node(_fileRange) {}

	virtual ~IsEntity() = default;

	bool is_entity() const final { return true; }

	virtual void create_entity(ir::Mod* parent, ir::Ctx* irCtx) = 0;

	virtual void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) = 0;

	virtual void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) = 0;
};

} // namespace qat::ast

#endif
