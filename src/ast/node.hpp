#ifndef QAT_AST_NODE_HPP
#define QAT_AST_NODE_HPP

#include "../IR/context.hpp"
#include "../IR/types/array.hpp"
#include "../IR/types/choice.hpp"
#include "../IR/types/float.hpp"
#include "../IR/types/function.hpp"
#include "../IR/types/integer.hpp"
#include "../IR/types/mix.hpp"
#include "../IR/types/native_type.hpp"
#include "../IR/types/pointer.hpp"
#include "../IR/types/reference.hpp"
#include "../IR/types/string_slice.hpp"
#include "../IR/types/struct_type.hpp"
#include "../IR/types/tuple.hpp"
#include "../IR/types/unsigned.hpp"
#include "../show.hpp"
#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"
#include "../utils/json.hpp"
#include "./node_type.hpp"

namespace qat::ast {

struct VisibilitySpec {
	VisibilityKind kind;
	FileRange      range;

	useit String to_string() const {
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
	useit Json to_json() const { return Json()._("visibilityKind", kind_to_string(kind))._("fileRange", range); }
};

class Commentable {
  public:
	Maybe<Pair<String, FileRange>> commentValue;
	useit bool                     hasCommentValue() const { return commentValue.has_value(); }
};

#define COMMENTABLE_FUNCTIONS                                                                                          \
	useit bool         isCommentable() const final { return true; }                                                    \
	useit Commentable* asCommentable() final { return (Commentable*)this; }

#define UPDATE_DEPS(x)               x->update_dependencies(phase, ir::DependType::complete, ent, ctx)
#define UPDATE_DEPS_CUSTOM(x, depTy) x->update_dependencies(phase, ir::DependType::depTy, ent, ctx)

// Node is the base class for all AST members of the language, and it
// requires a FileRange instance that indicates its position in the
// corresponding file
class Node {
  private:
	static Vec<Node*> allNodes;

  public:
	FileRange fileRange;

	explicit Node(FileRange _fileRange);
	virtual ~Node() = default;
	useit virtual bool         isCommentable() const { return false; }
	useit virtual Commentable* asCommentable() { return nullptr; }
	useit virtual bool         isPrerunNode() const { return false; }

	virtual void create_module(ir::Mod* mod, ir::Ctx* irCtx) const {}
	virtual void handle_fs_brings(ir::Mod* mod, ir::Ctx* irCtx) const {}

	useit virtual bool is_entity() const { return false; }

	useit virtual Json     to_json() const  = 0;
	useit virtual NodeType nodeType() const = 0;
	static void            clear_all();
};

class IsEntity : public Node {
  public:
	ir::EntityState* entityState = nullptr;

	IsEntity(FileRange _fileRange) : Node(_fileRange) {}
	virtual ~IsEntity() = default;

	useit bool is_entity() const final { return true; }

	virtual void create_entity(ir::Mod* parent, ir::Ctx* irCtx)                 = 0;
	virtual void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx)    = 0;
	virtual void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) = 0;
};

class HolderNode : public Node {
  private:
	Node* node;

  public:
	explicit HolderNode(Node* _node) : Node(_node->fileRange), node(_node) {}

	// NOLINTNEXTLINE(misc-unused-parameters)
	useit Json     to_json() const final { return node->to_json(); }
	useit NodeType nodeType() const final { return NodeType::HOLDER; }
};

} // namespace qat::ast

#endif
