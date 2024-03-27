#ifndef QAT_CLI_COLOR_HPP
#define QAT_CLI_COLOR_HPP

namespace qat::cli {

enum class Color { white, green, red, cyan, blue, yellow, orange, purple, grey, reset };
const char* get_color(Color value);
const char* get_bg_color(Color value);

} // namespace qat::cli

namespace colors {

const auto reset("\033[0m");
const auto bold("\033[1m");
const auto dim("\033[2m");
const auto italic("\033[3m");

} // namespace colors

#endif