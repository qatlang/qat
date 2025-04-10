#ifndef QAT_UTILS_LAUNCH_PROCESS_HPP
#define QAT_UTILS_LAUNCH_PROCESS_HPP

#include "helpers.hpp"
#include "macros.hpp"

namespace qat {

useit int run_command_get_code(String command, Vec<String> const& args);

useit Pair<int, String> run_command_get_stdout(String command, Vec<String> const& args);

useit Pair<int, String> run_command_get_output(String command, Vec<String> const& args);

useit int run_command_with_output(String command, Vec<String> const& args);

useit Pair<int, String> run_command_get_stderr(String command, Vec<String> const& args);

useit std::tuple<int, String, String> run_command_get_stdout_and_stderr(String command, Vec<String> const& args);

} // namespace qat

#endif
