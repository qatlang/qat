#ifndef QAT_SHOW_HPP
#define QAT_SHOW_HPP

#include "./cli/color.hpp"
#include <iostream>

#if NDEBUG
#define SHOW(val)
#define HIGHLIGHT(val)
#else
#define SHOW(val)                                                                                                      \
  std::cout << qat::cli::get_color(qat::cli::Color::cyan) << val << qat::cli::get_color(qat::cli::Color::reset) << "\n";
#define HIGHLIGHT(col, val)                                                                                            \
  std::cout << qat::cli::get_color(qat::cli::Color::yellow) << val << qat::cli::get_color(qat::cli::Color::reset)      \
            << "\n";
#endif

#endif