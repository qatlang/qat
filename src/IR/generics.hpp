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
	static GenericToFill* GetPrerun(ir::PrerunValue* constVal, FileRangePtr range);
	static GenericToFill* GetType(ir::Type* type, FileRangePtr range);

	bool             is_prerun() const;
	ir::PrerunValue* as_prerun() const;

	bool      is_type() const;
	ir::Type* as_type() const;

	FileRangePtr get_range() const;

	String to_string() const;
};

class GenericArgument {
  protected:
	Identifier   name;
	GenericKind  kind;
	FileRangePtr range;

	GenericArgument(Identifier name, GenericKind kind, FileRangePtr range);

  public:
	Identifier get_name() const;

	FileRangePtr get_range() const;

	bool is_same(const String& name) const;

	bool          is_typed() const;
	TypedGeneric* as_typed() const;

	bool           is_prerun() const;
	PrerunGeneric* as_prerun() const;

	bool is_equal_to(ir::Ctx* irCtx, GenericToFill* fill) const;

	String to_string() const;

	virtual ~GenericArgument() = default;
};

class TypedGeneric : public GenericArgument {
	ir::Type* type;

  public:
	TypedGeneric(Identifier name, ir::Type* type, FileRangePtr range);
	static TypedGeneric* get(Identifier name, ir::Type* type, FileRangePtr range);

	ir::Type* get_type() const;
};

class PrerunGeneric : public GenericArgument {
	ir::PrerunValue* constant;

  public:
	PrerunGeneric(Identifier name, ir::PrerunValue* constant, FileRangePtr range);
	static PrerunGeneric* get(Identifier name, ir::PrerunValue* type, FileRangePtr range);

	ir::PrerunValue* get_expression() const;

	ir::Type* get_type() const;
};

} // namespace qat::ir

#endif
