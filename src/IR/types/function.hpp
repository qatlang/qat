#ifndef QAT_IR_TYPES_FUNCTION_TYPE_HPP
#define QAT_IR_TYPES_FUNCTION_TYPE_HPP

#include "../../utils/qat_region.hpp"
#include "./qat_type.hpp"
#include "./type_kind.hpp"

#include <llvm/IR/LLVMContext.h>
#include <string>

namespace qat::ir {

enum class ArgumentKind : u8 { NORMAL, MEMBER };

class ArgumentType {
	Maybe<String> name;
	Type*         type;
	bool          variability;
	ArgumentKind  kind;

  public:
	ArgumentType(ArgumentKind _kind, Maybe<String> _name, Type* _type, bool _isVar)
	    : name(std::move(_name)), type(_type), variability(_isVar), kind(_kind) {}

	static ArgumentType* create_normal(Type* type, Maybe<String> name, bool isVar) {
		return std::construct_at(OwnNormal(ArgumentType), ArgumentKind::NORMAL, std::move(name), type, isVar);
	}

	static ArgumentType* create_member(String name, Type* type = nullptr) {
		return std::construct_at(OwnNormal(ArgumentType), ArgumentKind::MEMBER, name, type, false);
	}

	bool is_same_as(ArgumentType* other) {
		if (kind != other->kind) {
			return false;
		}
		switch (kind) {
			case ArgumentKind::NORMAL: {
				return (variability == other->variability) && (type->is_same(other->type));
			}
			case ArgumentKind::MEMBER: {
				return (name.value() == other->name.value());
			}
		}
	}

	bool has_name() const { return name.has_value(); }

	String get_name() const { return name.value_or(""); }

	Type* get_type() const { return type; }

	ArgumentKind get_kind() const { return kind; }

	bool is_variable() const { return variability; }

	bool is_member_argument() const { return kind == ArgumentKind::MEMBER; }

	String to_string() const {
		switch (kind) {
			case ArgumentKind::NORMAL:
				return (variability ? "var " : "") + (name.has_value() ? (name.value() + " :: ") : "") +
				       type->to_string();
			case ArgumentKind::MEMBER:
				return "''" + name.value();
		}
	}
};

class ReturnType {
	Type* retTy;
	bool  isReturnSelfRef;

  public:
	ReturnType(Type* _retTy, bool _isReturnSelfRef);

	static ReturnType* get(Type* _retTy) { return std::construct_at(OwnNormal(ReturnType), _retTy, false); }

	static ReturnType* get(Type* _retTy, bool _isRetSelf) {
		return std::construct_at(OwnNormal(ReturnType), _retTy, _isRetSelf);
	}

	Type* get_type() const;

	bool is_return_self() const;

	String to_string() const;
};

enum class VariadicsKind {
	NORMAL,
	LEGACY,
	TYPED,
};

struct Variadics {
	VariadicsKind kind;
	ir::Type*     type = nullptr;

	String to_string() const {
		switch (kind) {
			case VariadicsKind::NORMAL:
				return "variadic";
			case VariadicsKind::LEGACY:
				return "variadic:legacy";
			case VariadicsKind::TYPED:
				return "variadic :: " + type->to_string();
		}
	}
};

class FunctionType final : public Type {
	ReturnType*        returnType;
	Vec<ArgumentType*> argTypes;
	Maybe<Variadics>   variadics;

  public:
	FunctionType(ReturnType* retType, Vec<ArgumentType*> argTypes, Maybe<Variadics> variadics, llvm::LLVMContext& ctx);

	static FunctionType* create(ReturnType* retTy, Vec<ArgumentType*> argTys, Maybe<Variadics> variadics,
	                            llvm::LLVMContext& llCtx) {
		return std::construct_at(OwnNormal(FunctionType), retTy, std::move(argTys), variadics, llCtx);
	}

	~FunctionType() final;

	ReturnType* get_return_type() const { return returnType; }

	ArgumentType* get_argument_type_at(u64 index) const { return argTypes[index]; }

	Vec<ArgumentType*> const& get_argument_types() const { return argTypes; }

	u64 get_argument_count() const { return argTypes.size(); }

	bool is_variadic() const { return variadics.has_value(); }

	Variadics get_variadics() const { return variadics.value(); }

	TypeKind type_kind() const final { return TypeKind::FUNCTION; }

	String to_string() const final;
};

} // namespace qat::ir

#endif
