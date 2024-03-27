#include "./sitter.hpp"
#include "cli/config.hpp"
#include "cli/logger.hpp"
#include "utils/qat_region.hpp"

int main(int count, const char** args) {
  using namespace qat;

  auto* cli = cli::Config::init(count, args);
  if (cli->should_exit()) {
    return 0;
  }
  auto* sitter = QatSitter::get();
  sitter->initialise();
  TrackedRegion::destroyMembers();
  QatRegion::destroyAllBlocks();
  return 0;
}
