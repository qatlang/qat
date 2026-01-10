#ifndef QAT_AST_PATTERNS_HPP
#define QAT_AST_PATTERNS_HPP

#include "../IR/prerun_function.hpp"
#include "../utils/file_range.hpp"
#include "../utils/identifier.hpp"
#include "../utils/qat_region.hpp"

#include <helpers/deque.hpp>
#include <variant>

namespace llvm {
class Value;
};

namespace qat::ir {
class Type;
class Value;
class PreBlock;
class Block;
} // namespace qat::ir

namespace qat::ast {

struct EmitCtx;

enum class PatternType {
	ARRAY,
	TUPLE,
	IS,
	NONE,
	NULL_PTR,
	CHARACTER,
	BYTE_CHARACTER,
	BOOLEAN,
	CHOICE,
	MIX,
	FLAG,
	OK,
	ERROR,
	RANGE,
	INTEGER,
	FLOAT,
	STRING_LITERAL,
	COMPARISON,
	CHAIN,
	ELLIPSIS,
};

inline bool pattern_supports_chaining(PatternType type) {
	switch (type) {
		case PatternType::BOOLEAN:
		case PatternType::CHOICE:
		case PatternType::MIX:
		case PatternType::FLAG:
		case PatternType::RANGE:
		case PatternType::CHARACTER:
		case PatternType::BYTE_CHARACTER:
		case PatternType::STRING_LITERAL:
		case PatternType::INTEGER:
		case PatternType::FLOAT:
		case PatternType::COMPARISON:
			return true;
		default:
			return false;
	}
}

enum class PatternFillType {
	POSITIVE,
	NEGATIVE,
	MESSAGE,
	COMPLETE,
	NONE,
};

struct PatternFill {
	PatternFillType fillType;
	Vec<String>     fills;
	ir::Type*       type;

	Vec<PatternFill*> childFills;

	PatternFill(ir::Type* _type) : fillType(PatternFillType::NONE), type(_type) {}

	static PatternFill* create_for_type(EmitCtx* ctx, ir::Type* type);
};

struct ConditionSlot {
	Vec<llvm::Value*> conditions;
};

struct MatchArm {
	bool  isRef;
	bool  isRefVar;
	void* conditionBlock;
	void* bodyBlock;

	Deque<ConditionSlot> conditions;

	MatchArm(bool _isRef, bool _isRefVar, void* _condBlock, void* _bodyBlock)
	    : isRef(_isRef), isRefVar(_isRefVar), conditionBlock(_condBlock), bodyBlock(_bodyBlock) {
		conditions.push_back(ConditionSlot{.conditions = {}});
	}

	ConditionSlot& get_slot() { return conditions.back(); }

	ir::Block* get_condition_block() const { return (ir::Block*)conditionBlock; }

	ir::PreBlock* as_prerun_block() const { return (ir::PreBlock*)bodyBlock; }

	ir::Block* as_block() const { return (ir::Block*)bodyBlock; }
};

struct Pattern {
	PatternType  type;
	FileRangePtr range;

	Pattern(PatternType _type, FileRangePtr _range) : type(_type), range(std::move(_range)) {}

	void precheck(PatternFill* fill, EmitCtx* ctx) const;

	virtual void check(PatternFill* fill, bool isPartOfChain, MatchArm& arm, EmitCtx* ctx) const = 0;

	virtual void match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const = 0;

	virtual String to_string() const = 0;
};

enum class BindingType {
	VALUED,
	NORMAL,
	VARIATION,
};

struct PatternBinding {
	BindingType  bindType;
	Identifier   name;
	FileRangePtr range;

	static PatternBinding create(BindingType bindType, Identifier name, FileRangePtr range) {
		return PatternBinding{
		    .bindType = bindType,
		    .name     = std::move(name),
		    .range    = std::move(range),
		};
	}

	bool is_normal() const { return bindType == BindingType::NORMAL; }

	bool is_var() const { return bindType == BindingType::VARIATION; }

	bool is_valued() const { return bindType == BindingType::VALUED; }

	String to_string() const { return String(is_var() ? "var " : (is_valued() ? "use " : "")) + name.value; }
};

struct PatternChild {
	std::variant<Pattern*, PatternBinding> child;

	PatternChild(Pattern* _pattern) : child(std::in_place_index<0>, std::move(_pattern)) {}

	PatternChild(PatternBinding _binding) : child(std::in_place_index<1>, std::move(_binding)) {}

	bool is_pattern() const { return child.index() == 0; }

	Pattern* as_pattern() const { return std::get<0>(child); }

	bool is_binding() const { return child.index() == 1; }

	PatternBinding const& as_binding() const { return std::get<1>(child); }

	FileRangePtr get_range() { return is_pattern() ? as_pattern()->range : as_binding().range; }

	void check(PatternFill* fill, bool isPartOfChain, MatchArm& arm, EmitCtx* ctx) const;

	void match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const;

	String to_string() const {
		if (is_pattern()) {
			return as_pattern()->to_string();
		} else {
			return as_binding().to_string();
		}
	}
};

struct PatternArray final : public Pattern {
	Vec<PatternChild>                patterns;
	Maybe<Pair<usize, FileRangePtr>> ellipsis;

	mutable Vec<usize> patternIndices;

  public:
	PatternArray(Vec<PatternChild> _patterns, Maybe<Pair<usize, FileRangePtr>> _ellipsis, FileRangePtr _fileRange)
	    : Pattern(PatternType::ARRAY, std::move(_fileRange)), patterns(std::move(_patterns)),
	      ellipsis(std::move(_ellipsis)) {
		patternIndices.reserve(patterns.size());
	}

	static PatternArray* create(Vec<PatternChild> patterns, Maybe<Pair<usize, FileRangePtr>> ellipsis,
	                            FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PatternArray), std::move(patterns), std::move(ellipsis),
		                         std::move(fileRange));
	}

	void check(PatternFill* fill, bool isPartOfChain, MatchArm& arm, EmitCtx* ctx) const final;

	void match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const final;

	String to_string() const final;
};

struct PatternChoice final : public Pattern {
	Identifier name;

  public:
	PatternChoice(Identifier _name, FileRangePtr _fileRange)
	    : Pattern(PatternType::CHOICE, std::move(_fileRange)), name(std::move(_name)) {}

	static PatternChoice* create(Identifier name, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PatternChoice), std::move(name), std::move(fileRange));
	}

	void check(PatternFill* fill, bool isPartOfChain, MatchArm& arm, EmitCtx* ctx) const final;

	void match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const final;

	String to_string() const final { return "::" + name.value; }
};

struct PatternMix final : public Pattern {
	Identifier          name;
	Maybe<PatternChild> child;

  public:
	PatternMix(Identifier _name, PatternChild _child, FileRangePtr _fileRange)
	    : Pattern(PatternType::MIX, std::move(_fileRange)), name(std::move(_name)), child(std::move(_child)) {}

	static PatternMix* create(Identifier name, PatternChild child, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PatternMix), std::move(name), std::move(child), std::move(fileRange));
	}

	void check(PatternFill* fill, bool isPartOfChain, MatchArm& arm, EmitCtx* ctx) const final;

	void match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const final;

	String to_string() const final {
		return "::" + name.value + "(" + (child.has_value() ? child->to_string() : "") + ")";
	}
};

struct PatternChain final : public Pattern {
	Vec<Pattern> patterns;

  public:
	PatternChain(Vec<Pattern> _patterns, FileRangePtr _fileRange)
	    : Pattern(PatternType::CHAIN, std::move(_fileRange)), patterns(std::move(_patterns)) {}

	~PatternChain() {}

	static PatternChain* create(Vec<Pattern> patterns, FileRangePtr range) {
		return std::construct_at(OwnNormal(PatternChain), std::move(patterns), std::move(range));
	}

	void check(PatternFill* fill, bool isPartOfChain, MatchArm& arm, EmitCtx* ctx) const final;

	void match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const final;

	String to_string() const final {
		String res;
		for (usize i = 0; i < patterns.size(); i++) {
			res += patterns[i].to_string();
			if (i != (patterns.size() - 1)) {
				res += " | ";
			}
		}
		return res;
	}
};

enum class FlagPatternKind : u8 {
	DEFAULT,
	VARIANTS,
	NONE,
};

struct PatternFlag final : public Pattern {
	Vec<Identifier> names;
	FlagPatternKind flagKind;

  public:
	PatternFlag(Vec<Identifier> _names, FlagPatternKind _flagKind, FileRangePtr _fileRange)
	    : Pattern(PatternType::FLAG, std::move(_fileRange)), names(std::move(_names)), flagKind(_flagKind) {}

	static PatternFlag* create(Vec<Identifier> names, FlagPatternKind flagKind, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PatternFlag), std::move(names), flagKind, std::move(fileRange));
	}

	void check(PatternFill* fill, bool isPartOfChain, MatchArm& arm, EmitCtx* ctx) const final;

	void match(PatternFill* fill, ir::Value* value, MatchArm& arm, EmitCtx* ctx) const final;

	String to_string() const final {
		switch (flagKind) {
			case FlagPatternKind::DEFAULT: {
				return "::{ default }";
			}
			case FlagPatternKind::NONE: {
				return "::{ none }";
			}
			case FlagPatternKind::VARIANTS: {
				String res = "::{ ";
				for (usize i = 0; i < names.size(); i++) {
					res += names[i].value;
					if (i != (names.size() - 1)) {
						res += ", ";
					}
				}
				res += " }";
			}
		}
		std::unreachable();
	}
};

struct PatternRest final : public Pattern {
  public:
	PatternRest(FileRangePtr _fileRange) : Pattern(PatternType::ELLIPSIS, std::move(_fileRange)) {}

	static PatternRest* create(FileRangePtr range) {
		return std::construct_at(OwnNormal(PatternRest), std::move(range));
	}

	void check(PatternFill* fill, bool isPartOfChain, MatchArm& arm, EmitCtx* ctx) const final;

	void match(PatternFill*, ir::Value*, MatchArm&, EmitCtx*) const final {}

	String to_string() const final { return "..."; }
};

} // namespace qat::ast

#endif
