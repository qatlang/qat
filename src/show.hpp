#ifndef QAT_SHOW_HPP
#define QAT_SHOW_HPP

#include "./cli/color.hpp"

#if NDEBUG
#define SHOW(val)
#else
#define SHOW(val) std::cout << colors::blue << val << colors::reset << "\n";
#endif

#endif