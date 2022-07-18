#include "./error.hpp"

void qat::cli::throw_error(std::string message, std::filesystem::path path) {
  std::cout << colors::red << "[ CLI ERROR ] " << colors::bold::green
            << path.string() << "\n"
            << colors::reset << "   " << message << std::endl;
  exit(0);
}