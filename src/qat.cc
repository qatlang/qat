#include "./sitter.hpp"
#include "cli/config.hpp"
#include "cli/logger.hpp"
#include "utils/qat_region.hpp"

int main(int count, const char** args) {
  using namespace qat;

  auto* cli = cli::Config::init(count, args);
  if (cli->should_exit()) {
    delete cli;
    return 0;
  }
  auto* sitter = QatSitter::get();
  sitter->initialise();
  delete sitter;
  delete cli;
  QatRegion::destroyAllBlocks();
  return 0;
}
