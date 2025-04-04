#ifndef QAT_IR_GENERICS_HPP
#define QAT_IR_GENERICS_HPP

#include "../utils/identifier.hpp"
#include "./types/qat_type.hpp"

namespace qat::ast {
class GenericAbstractType;
struct EmitCtx;
} // namespace qat::ast

namespace qat::ir {

enum class GenericKind {
	typedGeneric,
	prerunGeneric,
};

class TypedGeneric;
class PrerunGeneric;
class PrerunValue;
class GenericToFill;

void fill_generics(ast::EmitCtx* irCtx, Vec<ast::GenericAbstractType*>& genAbs, Vec<GenericToFill*>& fills,
                   FileRange const& fileRange);

class GenericToFill {
	void*       data = nullptr;
	GenericKind kind;
	FileRange   range;

  public:
	GenericToFill(void* _data, GenericKind _kind, FileRange _range);
	useit static GenericToFill* GetPrerun(ir::PrerunValue* constVal, FileRange range);
	useit static GenericToFill* GetType(ir::Type* type, FileRange range);

	useit bool is_prerun() const;
	useit ir::PrerunValue* as_prerun() const;

	useit bool is_type() const;
	useit ir::Type* as_type() const;

	useit FileRange get_range() const;

	useit String to_string() const;
};

class GenericArgument {
  protected:
	Identifier  name;
	GenericKind kind;
	FileRange   range;

	GenericArgument(Identifier name, GenericKind kind, FileRange range);

  public:
	useit Identifier get_name() const;
	useit FileRange  get_range() const;
	useit bool       is_same(const String& name) const;

	useit bool          is_typed() const;
	useit TypedGeneric* as_typed() const;

	useit bool           is_prerun() const;
	useit PrerunGeneric* as_prerun() const;

	useit bool is_equal_to(ir::Ctx* irCtx, GenericToFill* fill) const;

	useit String       to_string() const;
	useit virtual Json to_json() const = 0;
	virtual ~GenericArgument()         = default;
};

class TypedGeneric : public GenericArgument {
	ir::Type* type;

  public:
	TypedGeneric(Identifier name, ir::Type* type, FileRange range);
	useit static TypedGeneric* get(Identifier name, ir::Type* type, FileRange range);

	useit ir::Type* get_type() const;

	useit Json to_json() const final;
};

class PrerunGeneric : public GenericArgument {
	ir::PrerunValue* constant;

  public:
	PrerunGeneric(Identifier name, ir::PrerunValue* constant, FileRange range);
	useit static PrerunGeneric* get(Identifier name, ir::PrerunValue* type, FileRange range);

	useit ir::PrerunValue* get_expression() const;
	useit ir::Type* get_type() const;

	useit Json to_json() const final;
};

} // namespace qat::ir

#endif
