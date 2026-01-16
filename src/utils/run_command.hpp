#ifndef QAT_UTILS_LAUNCH_PROCESS_HPP
#define QAT_UTILS_LAUNCH_PROCESS_HPP

#include <helpers/many.hpp>
#include <helpers/pair.hpp>
#include <helpers/string.hpp>
#include <helpers/vec.hpp>

namespace qat {

int run_command_get_code(String command, Vec<String> const& args);

Pair<int, String> run_command_get_stdout(String command, Vec<String> const& args);

Pair<int, String> run_command_get_output(String command, Vec<String> const& args);

int run_command_with_output(String command, Vec<String> const& args);

Pair<int, String> run_command_get_stderr(String command, Vec<String> const& args);

Many<int, String, String> run_command_get_stdout_and_stderr(String command, Vec<String> const& args);

} // namespace qat

#endif
