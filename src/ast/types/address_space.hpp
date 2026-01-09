#ifndef QAT_AST_TYPES_ADDRESS_SPACE_HPP
#define QAT_AST_TYPES_ADDRESS_SPACE_HPP

#include "../../IR/types/address_space.hpp"
#include "../../utils/helpers.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/macros.hpp"

namespace qat::ast {

class PrerunExpression;
struct EmitCtx;

struct AddressSpace {
	Identifier        name;
	PrerunExpression* value;
	FileRangePtr      fileRange;

	useit ir::AddressSpace to_ir(EmitCtx* ctx) const;

	useit String to_string() const;
};

} // namespace qat::ast

#endif
