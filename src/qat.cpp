#include "./CLI/config.hpp"

int main(int count, const char **args) {
  auto cli = qat::CLI::Config::init(count, args);
  if (cli.should_exit()) {
    return 0;
  }
  if (cli.is_compile()) {
    auto paths = cli.get_paths();
  }
  std::atexit(qat::CLI::Config::destroy);
  std::at_quick_exit(qat::CLI::Config::destroy);
  return 0;
}