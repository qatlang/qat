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
                   FileRangePtr fileRange);

class GenericToFill {
	void*        data = nullptr;
	GenericKind  kind;
	FileRangePtr range;

  public:
	GenericToFill(void* _data, GenericKind _kind, FileRangePtr _range);
	useit static GenericToFill* GetPrerun(ir::PrerunValue* constVal, FileRangePtr range);
	useit static GenericToFill* GetType(ir::Type* type, FileRangePtr range);

	useit bool is_prerun() const;
	useit ir::PrerunValue* as_prerun() const;

	useit bool is_type() const;
	useit ir::Type* as_type() const;

	useit FileRangePtr get_range() const;

	useit String to_string() const;
};

class GenericArgument {
  protected:
	Identifier   name;
	GenericKind  kind;
	FileRangePtr range;

	GenericArgument(Identifier name, GenericKind kind, FileRangePtr range);

  public:
	useit Identifier get_name() const;

	useit FileRangePtr get_range() const;

	useit bool is_same(const String& name) const;

	useit bool          is_typed() const;
	useit TypedGeneric* as_typed() const;

	useit bool           is_prerun() const;
	useit PrerunGeneric* as_prerun() const;

	useit bool is_equal_to(ir::Ctx* irCtx, GenericToFill* fill) const;

	useit String to_string() const;

	useit virtual Json to_json() const = 0;

	virtual ~GenericArgument() = default;
};

class TypedGeneric : public GenericArgument {
	ir::Type* type;

  public:
	TypedGeneric(Identifier name, ir::Type* type, FileRangePtr range);
	useit static TypedGeneric* get(Identifier name, ir::Type* type, FileRangePtr range);

	useit ir::Type* get_type() const;

	useit Json to_json() const final;
};

class PrerunGeneric : public GenericArgument {
	ir::PrerunValue* constant;

  public:
	PrerunGeneric(Identifier name, ir::PrerunValue* constant, FileRangePtr range);
	useit static PrerunGeneric* get(Identifier name, ir::PrerunValue* type, FileRangePtr range);

	useit ir::PrerunValue* get_expression() const;
	useit ir::Type* get_type() const;

	useit Json to_json() const final;
};

} // namespace qat::ir

#endif
