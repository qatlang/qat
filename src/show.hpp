#ifndef QAT_SHOW_HPP
#define QAT_SHOW_HPP

#include "./cli/color.hpp"
#include "./utils/logger.hpp"
#include <iostream>

#define HLIGHT(col, val) colors::col << val << colors::reset
#if NDEBUG
#define SHOW(val)
#define HIGHLIGHT(val)
#else
#define SHOW(val)           qat::Logger::get()->out << colors::blue << val << colors::reset << "\n";
#define HIGHLIGHT(col, val) qat::Logger::get()->out << colors::col << val << colors::reset << "\n";
#endif

#endif