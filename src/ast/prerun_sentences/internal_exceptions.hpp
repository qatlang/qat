#ifndef QAT_PRERUN_INTERNAL_EXCEPTIONS_HPP
#define QAT_PRERUN_INTERNAL_EXCEPTIONS_HPP

#include <helpers/maybe.hpp>
#include <helpers/string.hpp>

namespace qat {

namespace ir {
class PrerunValue;
};

struct InternalPrerunBreak {
	Maybe<String> tag;
};

struct InternalPrerunContinue {
	Maybe<String> tag;
};

struct InternalPrerunGive {
	ir::PrerunValue* value;
};

} // namespace qat

#endif
