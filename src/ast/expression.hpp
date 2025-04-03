#ifndef QAT_AST_EXPRESSION_HPP
#define QAT_AST_EXPRESSION_HPP

#include "./emit_ctx.hpp"
#include "./node.hpp"

namespace qat::ast {

#define LOCAL_DECL_COMPATIBLE_FUNCTIONS                                                                                \
	useit bool                 isLocalDeclCompatible() const final { return true; }                                    \
	useit LocalDeclCompatible* asLocalDeclCompatible() final { return (LocalDeclCompatible*)this; }

#define IN_PLACE_CREATABLE_FUNCTIONS                                                                                   \
	useit bool              isInPlaceCreatable() const final { return true; }                                          \
	useit InPlaceCreatable* asInPlaceCreatable() final { return (InPlaceCreatable*)this; }

#define TYPE_INFERRABLE_FUNCTIONS                                                                                      \
	useit bool            has_type_inferrance() const final { return true; }                                           \
	useit TypeInferrable* as_type_inferrable() final { return (TypeInferrable*)this; }

class LocalDeclCompatible {
  public:
	Maybe<Identifier> irName;
	bool              isVar = false;

	useit bool isLocalDecl() const { return irName.has_value(); }
};

class InPlaceCreatable {
  public:
	ir::Value* createIn           = nullptr;
	bool       ignoreCreateInType = false;

	useit bool type_check_create_in(ir::Type* _type) const {
		return ignoreCreateInType ||
		       (createIn->is_ghost_ref() ? createIn->get_ir_type()->is_same(_type)
		                                 : createIn->get_ir_type()->as_ref()->get_subtype()->is_same(_type));
	}

	useit ir::Value* get_creation_result(ir::Ctx* irCtx, ir::Type* type, FileRange rangeVal) {
		auto created = createIn;
		unsetCreateIn();
		if (created->is_ref()) {
			return ir::Value::get(created->get_llvm(),
			                      ir::RefType::get(created->get_ir_type()->as_ref()->has_variability(), type, irCtx),
			                      false)
			    ->with_range(std::move(rangeVal));
		} else {
			return ir::Value::get(created->get_llvm(), type, created->is_variable())->with_range(std::move(rangeVal));
		}
	}

	useit bool canCreateIn() const { return createIn != nullptr; }

	void setCreateIn(ir::Value* _createIn) { createIn = _createIn; }

	void unsetCreateIn() { createIn = nullptr; }
};

struct FnAtEnd {
	std::function<void()> fn;

	FnAtEnd(std::function<void()> _fn) : fn(_fn) {}

	~FnAtEnd() { fn(); }
};

class TypeInferrable {
  public:
	ir::Type* inferredType = nullptr;

	useit bool is_type_inferred() const { return inferredType != nullptr; }

	void check_inferred_type(ir::Type* provided, EmitCtx* ctx, FileRange fileRange) const {
		if (inferredType && not inferredType->is_same(provided)) {
			ctx->Error("The type inferred for this expression is " + ctx->color(inferredType->to_string()) +
			               " which does not match with the provided type which is " + ctx->color(provided->to_string()),
			           fileRange);
		}
	}

	void set_inference_type(ir::Type* _type) {
		inferredType = _type->is_ref() ? _type->as_ref()->get_subtype() : _type;
	}
};

class Expression : public Node {
  public:
	Expression(FileRange _fileRange) : Node(std::move(_fileRange)) {}

	~Expression() override = default;

	useit virtual bool isLocalDeclCompatible() const { return false; }

	useit virtual LocalDeclCompatible* asLocalDeclCompatible() { return nullptr; }

	useit virtual bool isInPlaceCreatable() const { return false; }

	useit virtual InPlaceCreatable* asInPlaceCreatable() { return nullptr; }

	useit virtual bool has_type_inferrance() const { return false; }

	useit virtual TypeInferrable* as_type_inferrable() { return nullptr; }

	virtual void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
	                                 EmitCtx* ctx) = 0;

	useit virtual ir::Value* emit(EmitCtx* emitCtx) = 0;

	useit NodeType nodeType() const override = 0;
	useit Json     to_json() const override  = 0;
};

class PrerunExpression : public Expression {
  public:
	PrerunExpression(FileRange _fileRange) : Expression(std::move(_fileRange)) {}

	~PrerunExpression() override = default;

	useit ir::PrerunValue* emit(EmitCtx* emitCtx) override = 0;
	useit NodeType         nodeType() const override       = 0;
	useit Json             to_json() const override        = 0;
	useit virtual String   to_string() const               = 0;
};

} // namespace qat::ast

#endif
