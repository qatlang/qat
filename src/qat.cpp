#include "./error.hpp"
#include "./qat_sitter.hpp"

int main(int count, const char **args) {
  using qat::CLI::Config;

  qat::initAllErrors();
  auto cli = Config::init(count, args);
  if (cli->shouldExit()) {
    return 0;
  }
  auto sitter = qat::QatSitter();
  sitter.init();
  Config::destroy();
  return 0;
}