#ifndef QAT_AST_DEFINE_STRUCT_HPP
#define QAT_AST_DEFINE_STRUCT_HPP

#include "./expression.hpp"
#include "./types/qat_type.hpp"
#include "member_parent_like.hpp"
#include "meta_info.hpp"
#include "node.hpp"
#include "types/generic_abstract.hpp"

namespace qat::ast {

class DefineStructType final : public IsEntity, public Commentable, public MemberParentLike {
	friend class ir::GenericStructType;

  public:
	struct Member {
		Member(Type* _type, Identifier _name, bool _variability, Maybe<VisibilitySpec> _visibSpec,
		       Maybe<Expression*> _expression, FileRangePtr _fileRange)
		    : type(_type), name(_name), variability(_variability), visibSpec(_visibSpec), expression(_expression),
		      fileRange(_fileRange) {}

		static Member* create(Type* _type, Identifier _name, bool _variability, Maybe<VisibilitySpec> _visibSpec,
		                      Maybe<Expression*> _expression, FileRangePtr _fileRange) {
			return std::construct_at(OwnNormal(Member), _type, _name, _variability, _visibSpec, _expression,
			                         _fileRange);
		}

		Type*                 type;
		Identifier            name;
		bool                  variability;
		Maybe<VisibilitySpec> visibSpec;
		Maybe<Expression*>    expression;
		FileRangePtr          fileRange;
	};

	// Static member representation in the AST
	struct StaticMember {
		StaticMember(Type* _type, Identifier _name, bool _variability, Expression* _value,
		             Maybe<VisibilitySpec> _visibSpec, FileRangePtr _fileRange)
		    : type(_type), name(_name), variability(_variability), value(_value), visibSpec(_visibSpec),
		      fileRange(_fileRange) {}

		static StaticMember* create(Type* _type, Identifier _name, bool _variability, Expression* _value,
		                            Maybe<VisibilitySpec> _visibSpec, FileRangePtr _fileRange) {
			return std::construct_at(OwnNormal(StaticMember), _type, _name, _variability, _value, _visibSpec,
			                         _fileRange);
		}

		Type*                 type;
		Identifier            name;
		bool                  variability;
		Expression*           value;
		Maybe<VisibilitySpec> visibSpec;
		FileRangePtr          fileRange;
	};

  private:
	Identifier            name;
	PrerunExpression*     defineChecker;
	Vec<Member*>          members;
	Vec<StaticMember*>    staticMembers;
	Maybe<FileRangePtr>   simpleCopy;
	Maybe<FileRangePtr>   simpleMove;
	Maybe<VisibilitySpec> visibSpec;
	Maybe<MetaInfo>       metaInfo;

	Vec<ast::GenericAbstractType*> generics;
	PrerunExpression*              genericConstraint;
	mutable Vec<ir::OpaqueType*>   opaquedTypes;
	bool                           hasOpaque() const;
	void                           setOpaque(ir::OpaqueType* opq) const;
	ir::OpaqueType*                get_opaque() const;
	void                           unsetOpaque() const;
	mutable ir::GenericStructType* genericStructType = nullptr;
	mutable ir::StructType*        resultType        = nullptr;
	mutable Maybe<bool>            checkResult;
	Maybe<ir::MetaInfo>            metaIR;

  public:
	DefineStructType(Identifier _name, PrerunExpression* _checker, Maybe<VisibilitySpec> _visibSpec,
	                 FileRangePtr _fileRange, Vec<ast::GenericAbstractType*> _generics,
	                 PrerunExpression* _genericConstraint, Maybe<MetaInfo> _metaInfo)
	    : IsEntity(_fileRange), name(_name), defineChecker(_checker), visibSpec(_visibSpec), metaInfo(_metaInfo),
	      generics(_generics), genericConstraint(_genericConstraint) {}

	static DefineStructType* create(Identifier _name, PrerunExpression* _checker, Maybe<VisibilitySpec> _visibSpec,
	                                FileRangePtr _fileRange, Vec<ast::GenericAbstractType*> _generics,
	                                PrerunExpression* _constraint, Maybe<MetaInfo> _metaInfo) {
		return std::construct_at(OwnNormal(DefineStructType), _name, _checker, _visibSpec, _fileRange, _generics,
		                         _constraint, _metaInfo);
	}

	COMMENTABLE_FUNCTIONS

	void addMember(Member* mem);
	void addStaticMember(StaticMember* stm);

	void create_opaque(ir::Mod* mod, ir::Ctx* irCtx);

	ir::StructType* create_type(Vec<ir::GenericToFill*> const& genericsToFill, ir::Mod* mod, ir::Ctx* irCtx) const;

	void create_type_definitions(ir::StructType* resultTy, ir::Mod* mod, ir::Ctx* irCtx);

	void setup_type(ir::Mod* mod, ir::Ctx* irCtx);

	void do_define(ir::StructType* resultTy, ir::Mod* mod, ir::Ctx* irCtx);

	void do_emit(ir::StructType* resultTy, ir::Ctx* irCtx);

	bool has_simple_copy() { return simpleCopy.has_value(); }

	void set_simple_copy(FileRangePtr range) { simpleCopy = std::move(range); }

	bool has_simple_move() { return simpleMove.has_value(); }

	void set_simple_move(FileRangePtr range) { simpleMove = std::move(range); }

	bool is_generic() const;

	bool has_default_constructor() const;

	bool has_destructor() const;

	bool has_copy_constructor() const;

	bool has_move_constructor() const;

	bool has_copy_assignment() const;

	bool has_move_assignment() const;

	bool is_define_struct_type() const final { return true; }

	DefineStructType* as_define_struct_type() final { return this; }

	void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;

	void update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) final;

	void do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) final;

	NodeType nodeType() const final { return NodeType::DEFINE_STRUCT_TYPE; }

	~DefineStructType() final;
};

} // namespace qat::ast

#endif
