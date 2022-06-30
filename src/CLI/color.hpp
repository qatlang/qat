#ifndef QAT_CLI_COLOR_HPP
#define QAT_CLI_COLOR_HPP

/**
 * No License for this file. Go Nuts!
 *
 */

#include <string>

namespace colors {
const auto reset("\e[0m");
const auto bold_("\e[1m");

const auto black("\e[0;30m");
const auto red("\e[0;31m");
const auto green("\e[0;32m");
const auto yellow("\e[0;33m");
const auto blue("\e[0;34m");
const auto purple("\e[0;35m");
const auto cyan("\e[0;36m");
const auto white("\e[0;37m");

namespace bold {
const auto black("\e[1;30m");
const auto red("\e[1;31m");
const auto green("\e[1;32m");
const auto yellow("\e[1;33m");
const auto blue("\e[1;34m");
const auto purple("\e[1;35m");
const auto cyan("\e[1;36m");
const auto white("\e[1;37m");
} // namespace bold

namespace underline {
const auto black("\e[4;30m");
const auto red("\e[4;31m");
const auto green("\e[4;32m");
const auto yellow("\e[4;33m");
const auto blue("\e[4;34m");
const auto purple("\e[4;35m");
const auto cyan("\e[4;36m");
const auto white("\e[4;37m");
} // namespace underline

namespace background {
const auto black("\e[40m");
const auto red("\e[41m");
const auto green("\e[42m");
const auto yellow("\e[43m");
const auto blue("\e[44m");
const auto purple("\e[45m");
const auto cyan("\e[46m");
const auto white("\e[47m");
} // namespace background

namespace highIntesity {
const auto black("\e[0;90m");
const auto red("\e[0;91m");
const auto green("\e[0;92m");
const auto yellow("\e[0;93m");
const auto blue("\e[0;94m");
const auto purple("\e[0;95m");
const auto cyan("\e[0;96m");
const auto white("\e[0;97m");
} // namespace highIntesity

namespace boldHighIntensity {
const auto black("\e[1;90m");
const auto red("\e[1;91m");
const auto green("\e[1;92m");
const auto yellow("\e[1;93m");
const auto blue("\e[1;94m");
const auto purple("\e[1;95m");
const auto cyan("\e[1;96m");
const auto white("\e[1;97m");
} // namespace boldHighIntensity

namespace highIntensityBackground {
const auto black("\e[0;100m");
const auto red("\e[1;101m");
const auto green("\e[1;102m");
const auto yellow("\e[1;103m");
const auto blue("\e[1;104m");
const auto purple("\e[1;105m");
const auto cyan("\e[1;106m");
const auto white("\e[1;107m");
} // namespace highIntensityBackground

} // namespace colors

#endif