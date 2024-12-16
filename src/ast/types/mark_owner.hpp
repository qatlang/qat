#ifndef QAT_AST_TYPES_MARK_OWNER_HPP
#define QAT_AST_TYPES_MARK_OWNER_HPP

#include "../../IR/types/pointer.hpp"
#include "../../utils/file_range.hpp"
#include "../../utils/macros.hpp"

namespace qat::ir {
class Type;
}

namespace qat::ast {

class EmitCtx;

enum class MarkOwnType {
	heap,
	type,
	typeParent,
	function,
	anonymous,
	region,
	anyRegion,
};

useit ir::MarkOwner get_mark_owner(EmitCtx* ctx, MarkOwnType ownType, Maybe<ir::Type*> ownerVal, FileRange fileRange);

useit String mark_owner_to_string(MarkOwnType ownType);

} // namespace qat::ast

#endif
