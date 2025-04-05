#ifndef QAT_AST_TYPES_PTR_OWNER_HPP
#define QAT_AST_TYPES_PTR_OWNER_HPP

#include "../../IR/types/pointer.hpp"
#include "../../utils/file_range.hpp"
#include "../../utils/macros.hpp"

namespace qat::ir {
class Type;
}

namespace qat::ast {

class EmitCtx;

enum class PtrOwnType {
	heap,
	type,
	typeParent,
	function,
	anonymous,
	region,
	anyRegion,
};

useit ir::PtrOwner get_ptr_owner(EmitCtx* ctx, PtrOwnType ownType, Maybe<ir::Type*> ownerVal, FileRange fileRange);

useit String ptr_owner_to_string(PtrOwnType ownType);

} // namespace qat::ast

#endif
