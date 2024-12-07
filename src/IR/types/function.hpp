#ifndef QAT_IR_TYPES_FUNCTION_TYPE_HPP
#define QAT_IR_TYPES_FUNCTION_TYPE_HPP

#include "./qat_type.hpp"
#include "./type_kind.hpp"

#include <llvm/IR/LLVMContext.h>
#include <string>

namespace qat::ir {

enum class ArgumentKind : u8 { NORMAL, MEMBER, VARIADIC };

class ArgumentType {
  private:
	Maybe<String> name;
	Type*		  type;
	bool		  variability;
	ArgumentKind  kind;

	ArgumentType(ArgumentKind _kind, Maybe<String> _name, Type* _type, bool _isVar)
		: name(std::move(_name)), type(_type), variability(_isVar), kind(_kind) {}

  public:
	useit static ArgumentType* create_normal(Type* type, Maybe<String> name, bool isVar) {
		return new ArgumentType(ArgumentKind::NORMAL, std::move(name), type, isVar);
	}

	useit static ArgumentType* create_member(String name, Type* type = nullptr) {
		return new ArgumentType(ArgumentKind::MEMBER, name, type, false);
	}

	useit static ArgumentType* create_variadic(Maybe<String> name) {
		return new ArgumentType(ArgumentKind::VARIADIC, std::move(name), nullptr, false);
	}

	useit bool is_same_as(ArgumentType* other) {
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
			case ArgumentKind::VARIADIC: {
				return (type != nullptr) ? ((other->type != nullptr) && type->is_same(other->type))
										 : (other->type == nullptr);
			}
		}
	}

	useit bool has_name() const { return name.has_value(); }

	useit String get_name() const { return name.value(); }

	useit Type* get_type() const { return type; }

	useit ArgumentKind get_kind() const { return kind; }

	useit bool is_variable() const { return variability; }

	useit bool is_member_argument() const { return kind == ArgumentKind::MEMBER; }

	useit bool is_variadic_argument() const { return kind == ArgumentKind::VARIADIC; }

	useit Json to_json() const {
		return Json()
			._("hasName", name.has_value())
			._("name", name.has_value() ? name.value() : JsonValue())
			._("hasType", type != nullptr)
			._("type", type ? type->get_id() : JsonValue())
			._("isVar", variability)
			._("kind",
			   kind == ArgumentKind::MEMBER ? "member" : (kind == ArgumentKind::NORMAL ? "normal" : "variadic"));
	}

	useit String to_string() const {
		switch (kind) {
			case ArgumentKind::NORMAL:
				return (variability ? "var " : "") + (name.has_value() ? (name.value() + " :: ") : "") +
					   type->to_string();
			case ArgumentKind::MEMBER:
				return "''" + name.value();
			case ArgumentKind::VARIADIC:
				return "variadic " + (name.has_value() ? name.value() : "") +
					   (type ? (" :: " + type->to_string()) : "");
		}
	}
};

class ReturnType {
  private:
	Type* retTy;
	bool  isReturnSelfRef;

	ReturnType(Type* _retTy, bool _isReturnSelfRef);

  public:
	useit static ReturnType* get(Type* _retTy);
	useit static ReturnType* get(Type* _retTy, bool _isRetSelf);

	useit Type*	 get_type() const;
	useit bool	 is_return_self() const;
	useit String to_string() const;
};

class FunctionType final : public Type {
	ReturnType*		   returnType;
	Vec<ArgumentType*> argTypes;
	bool			   isVariadicArgs;

  public:
	FunctionType(ReturnType* _retType, Vec<ArgumentType*> _argTypes, llvm::LLVMContext& ctx);

	useit static FunctionType* create(ReturnType* retTy, Vec<ArgumentType*> argTys, llvm::LLVMContext& llCtx) {
		return new FunctionType(retTy, std::move(argTys), llCtx);
	}

	~FunctionType() final;

	useit ReturnType* get_return_type();

	useit ArgumentType* get_argument_type_at(u64 index);

	useit Vec<ArgumentType*> get_argument_types() const;

	useit u64 get_argument_count() const;

	useit bool is_variadic() const { return isVariadicArgs; }

	useit TypeKind type_kind() const final { return TypeKind::function; }

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
