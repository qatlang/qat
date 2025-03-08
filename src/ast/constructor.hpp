#ifndef QAT_AST_CONSTRUCTOR_PROTOTYPE_HPP
#define QAT_AST_CONSTRUCTOR_PROTOTYPE_HPP

#include "../IR/context.hpp"
#include "../IR/types/struct_type.hpp"
#include "./argument.hpp"
#include "./meta_info.hpp"
#include "./node.hpp"
#include "member_parent_like.hpp"
#include "sentence.hpp"

namespace qat::ast {

enum class ConstructorType {
	normal,
	copy,
	Default,
	move,
};

class ConstructorPrototype {
	friend class ConstructorDefinition;
	friend class DefineStructType;

  private:
	Vec<Argument*>        arguments;
	Maybe<VisibilitySpec> visibSpec;
	ConstructorType       type;
	Maybe<Identifier>     argName;
	FileRange             nameRange;
	FileRange             fileRange;

	PrerunExpression*    defineChecker;
	Maybe<ast::MetaInfo> metaInfo;

	mutable Maybe<bool>           checkResult;
	mutable Vec<ir::StructField*> presentMembers;
	mutable Vec<ir::StructField*> absentMembersWithDefault;
	mutable Vec<ir::StructField*> absentMembersWithoutDefault;

  public:
	ConstructorPrototype(ConstructorType _constrType, FileRange _nameRange, Vec<Argument*> _arguments,
	                     Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange, Maybe<Identifier> _argName,
	                     PrerunExpression* _defineChecker, Maybe<MetaInfo> _metaInfo)
	    : arguments(_arguments), visibSpec(_visibSpec), type(_constrType), argName(_argName), nameRange(_nameRange),
	      fileRange(_fileRange), defineChecker(_defineChecker), metaInfo(std::move(_metaInfo)) {}

	static ConstructorPrototype* Normal(FileRange nameRange, Vec<Argument*> args, Maybe<VisibilitySpec> visibSpec,
	                                    FileRange fileRange, PrerunExpression* defineChecker,
	                                    Maybe<MetaInfo> metaInfo) {
		return std::construct_at(OwnNormal(ConstructorPrototype), ConstructorType::normal, nameRange, std::move(args),
		                         visibSpec, std::move(fileRange), None, defineChecker, metaInfo);
	}

	static ConstructorPrototype* Default(Maybe<VisibilitySpec> visibSpec, FileRange nameRange, FileRange fileRange,
	                                     PrerunExpression* defineChecker, Maybe<MetaInfo> metaInfo) {
		return std::construct_at(OwnNormal(ConstructorPrototype), ConstructorType::Default, nameRange, Vec<Argument*>{},
		                         visibSpec, std::move(fileRange), None, defineChecker, metaInfo);
	}

	static ConstructorPrototype* Copy(Maybe<VisibilitySpec> visibSpec, FileRange nameRange, FileRange fileRange,
	                                  Identifier _argName, PrerunExpression* defineChecker, Maybe<MetaInfo> metaInfo) {
		return std::construct_at(OwnNormal(ConstructorPrototype), ConstructorType::copy, nameRange, Vec<Argument*>{},
		                         visibSpec, std::move(fileRange), _argName, defineChecker, metaInfo);
	}

	static ConstructorPrototype* Move(Maybe<VisibilitySpec> visibSpec, FileRange nameRange, FileRange fileRange,
	                                  Identifier _argName, PrerunExpression* defineChecker, Maybe<MetaInfo> metaInfo) {
		return std::construct_at(OwnNormal(ConstructorPrototype), ConstructorType::move, nameRange, Vec<Argument*>{},
		                         visibSpec, std::move(fileRange), _argName, defineChecker, metaInfo);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
		for (auto arg : arguments) {
			if (arg->get_type()) {
				UPDATE_DEPS(arg->get_type());
			}
		}
	}

	void       define(MethodState& state, ir::Ctx* irCtx);
	useit Json to_json() const;

	useit NodeType nodeType() const { return NodeType::CONVERTOR_PROTOTYPE; }

	~ConstructorPrototype();
};

class ConstructorDefinition {
	friend DefineStructType;
	friend DoSkill;

	Vec<Sentence*>        sentences;
	ConstructorPrototype* prototype;
	FileRange             fileRange;

  public:
	ConstructorDefinition(ConstructorPrototype* _prototype, Vec<Sentence*> _sentences, FileRange _fileRange)
	    : sentences(_sentences), prototype(_prototype), fileRange(_fileRange) {}

	useit static ConstructorDefinition* create(ConstructorPrototype* _prototype, Vec<Sentence*> _sentences,
	                                           FileRange _fileRange) {
		return std::construct_at(OwnNormal(ConstructorDefinition), _prototype, _sentences, _fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
		for (auto snt : sentences) {
			UPDATE_DEPS(snt);
		}
	}

	void  define(MethodState& state, ir::Ctx* irCtx);
	useit ir::Value* emit(MethodState& state, ir::Ctx* irCtx);
	useit Json       to_json() const;

	useit NodeType nodeType() const { return NodeType::MEMBER_DEFINITION; }
};

} // namespace qat::ast

#endif
