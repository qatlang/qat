#include "./sitter.hpp"
#include "cli/config.hpp"
#include "cli/logger.hpp"

int main(int count, const char** args) {
	using namespace qat;

	auto* cli = cli::Config::init(count, args);
	if (cli->should_exit()) {
		delete cli;
		return 0;
	}
	if (cli->is_workflow_analyse() || cli->is_workflow_build() || cli->is_workflow_bundle() || cli->is_workflow_run()) {
		cli->find_stdlib_and_toolchain();
	}
	auto* sitter = QatSitter::get();
	sitter->initialise();
	delete sitter;
	SHOW("Destroyed sitter")
	delete cli;
	SHOW("Destroyed cli")
	QatRegion::destroyAllBlocks();
	SHOW("Destroyed region blocks")
	return 0;
}
