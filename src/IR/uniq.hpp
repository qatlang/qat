#ifndef QAT_IR_ELEMENT_HPP
#define QAT_IR_ELEMENT_HPP

#include "../utils/macros.hpp"
#include "../utils/unique_id.hpp"
#include <string>

namespace qat::ir {

class Uniq {
	u64 id;

  public:
	explicit Uniq(u64 idVal) : id(idVal) {}
	Uniq() : id(utils::unique_id()) {}

	useit u64  get_id() const { return id; }
	useit bool isID(const u64& val) const { return (val == id); }
};

} // namespace qat::ir

#endif
