#ifndef QAT_AST_TYPES_PTR_OWNER_HPP
#define QAT_AST_TYPES_PTR_OWNER_HPP

#include "../../IR/types/pointer.hpp"
#include "../../utils/file_range.hpp"
#include "../../utils/macros.hpp"

namespace qat::ir {
class Type;
}

namespace qat::ast {

struct EmitCtx;
class Type;

enum class PtrOwnType {
	heap,
	type,
	typeParent,
	function,
	anonymous,
	region,
	anyRegion,
};

struct PtrOwner {
	PtrOwnType kind;
	ast::Type* candidate = nullptr;
	FileRange  range;

	useit static PtrOwner of_function(FileRange range) {
		return PtrOwner{.kind = PtrOwnType::function, .candidate = nullptr, .range = std::move(range)};
	}

	useit static PtrOwner of_heap(FileRange range) {
		return PtrOwner{.kind = PtrOwnType::heap, .candidate = nullptr, .range = std::move(range)};
	}

	useit static PtrOwner of_type(ast::Type* candidate, FileRange range) {
		return PtrOwner{.kind = PtrOwnType::type, .candidate = candidate, .range = std::move(range)};
	}

	useit static PtrOwner of_any_region(FileRange range) {
		return PtrOwner{.kind = PtrOwnType::region, .candidate = nullptr, .range = std::move(range)};
	}

	useit static PtrOwner of_region(ast::Type* region, FileRange range) {
		return PtrOwner{.kind = PtrOwnType::region, .candidate = region, .range = std::move(range)};
	}

	useit static PtrOwner of_type_parent(FileRange range) {
		return PtrOwner{.kind = PtrOwnType::typeParent, .candidate = nullptr, .range = std::move(range)};
	}

	useit static PtrOwner of_anonymous(FileRange range) {
		return PtrOwner{.kind = PtrOwnType::anonymous, .candidate = nullptr, .range = std::move(range)};
	}

	useit Json to_json() const;

	useit String to_string() const;
};

useit ir::PtrOwner get_ptr_owner(EmitCtx* ctx, PtrOwner owner, FileRange fileRange);

useit String ptr_owner_to_string(PtrOwnType ownType);

} // namespace qat::ast

#endif
