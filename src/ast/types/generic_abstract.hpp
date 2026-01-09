#ifndef QAT_AST_TYPES_GENERIC_ABSTRACT_HPP
#define QAT_AST_TYPES_GENERIC_ABSTRACT_HPP

#include "../../utils/identifier.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

enum class GenericKind : u8 {
	typedGeneric,
	prerunGeneric,
};

class TypedGenericAbstract;
class PrerunGenericAbstract;

class GenericAbstractType {
  protected:
	usize        index;
	Identifier   name;
	GenericKind  kind;
	FileRangePtr range;

	GenericAbstractType(usize _index, Identifier _name, GenericKind _kind, FileRangePtr _range)
	    : index(_index), name(std::move(_name)), kind(_kind), range(std::move(_range)) {
		ast::Type::generics.push_back(this);
	}

  public:
	usize        getIndex() const;
	Identifier   get_name() const;
	FileRangePtr get_range() const;

	GenericKind get_kind() const { return kind; }

	bool                   is_typed() const;
	TypedGenericAbstract*  as_typed() const;
	bool                   is_prerun() const;
	PrerunGenericAbstract* as_prerun() const;

	virtual void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                                 EmitCtx* ctx) = 0;

	virtual void emit(EmitCtx* ctx) const = 0;

	virtual bool hasDefault() const = 0;

	virtual bool isSet() const = 0;

	virtual void unset() const = 0;

	ir::GenericArgument* toIRGenericType() const;

	virtual ~GenericAbstractType() = default;
};

} // namespace qat::ast

#endif
