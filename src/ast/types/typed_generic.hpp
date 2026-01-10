#ifndef QAT_AST_NAMED_GENERIC_HPP
#define QAT_AST_NAMED_GENERIC_HPP

#include "../node.hpp"
#include "./generic_abstract.hpp"
#include "./qat_type.hpp"

#include <helpers/integers.hpp>
#include <helpers/maybe.hpp>
#include <helpers/vec.hpp>

namespace qat::ast {

class TypedGenericAbstract final : public GenericAbstractType {
	Maybe<ast::Type*>      defaultTypeAST;
	mutable ir::Type*      defaultType = nullptr;
	mutable Vec<ir::Type*> typeValue;

  public:
	TypedGenericAbstract(usize _index, Identifier _name, Maybe<ast::Type*> _defaultTy, FileRangePtr _fileRange)
	    : GenericAbstractType(_index, _name, GenericKind::typedGeneric, _fileRange), defaultTypeAST(_defaultTy) {}

	static TypedGenericAbstract* create(usize _index, Identifier _name, Maybe<ast::Type*> _defaultTy,
	                                    FileRangePtr _fileRange) {
		return std::construct_at(OwnNormal(TypedGenericAbstract), _index, std::move(_name), _defaultTy,
		                         std::move(_fileRange));
	}

	bool hasDefault() const final;

	ast::Type* getDefaultAST() const { return defaultTypeAST.value(); }

	ir::Type* getDefault() const;

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		if (defaultTypeAST.has_value()) {
			UPDATE_DEPS(defaultTypeAST.value());
		}
	}

	void emit(EmitCtx* ctx) const final;

	ir::Type*         get_type() const;
	ir::TypedGeneric* toIR() const;

	bool isSet() const final;
	void setType(ir::Type* typ) const;
	void unset() const final;

	~TypedGenericAbstract() final;
};

} // namespace qat::ast

#endif
