#ifndef QAT_AST_TYPES_QAT_TYPE_HPP
#define QAT_AST_TYPES_QAT_TYPE_HPP

#include "../../IR/context.hpp"
#include "../../show.hpp"
#include "../../utils/file_range.hpp"
#include "../emit_ctx.hpp"
#include "./type_kind.hpp"

namespace qat::ast {

class GenericAbstractType;

class Type {
	friend GenericAbstractType;

	static Vec<Type*> allTypes;

  protected:
	static Vec<GenericAbstractType*> generics;

  public:
	explicit Type(FileRange _fileRange);
	virtual ~Type() = default;

	FileRange fileRange;

	virtual void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
	                                 EmitCtx* ctx) {}

	useit virtual Maybe<usize> get_type_bitsize(EmitCtx* ctx) const { return None; }

	useit virtual ir::Type* emit(EmitCtx* ctx) = 0;

	useit virtual AstTypeKind type_kind() const = 0;

	useit virtual Json to_json() const = 0;

	useit virtual String to_string() const = 0;

	virtual void destroy() {}

	static void clear_all();
};

} // namespace qat::ast

#endif
