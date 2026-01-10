#include "./cli/config.hpp"
#include "./cli/logger.hpp"
#include "./sitter.hpp"

int main(int count, const char** args) {
	using namespace qat;
	static_assert(std::numeric_limits<float>::is_iec559, "float is expected to be 32 bits");
	static_assert(std::numeric_limits<double>::is_iec559, "double is expected to be 64 bits");

	auto* cli = cli::Config::initialise(count, args);
	if (cli->should_exit()) {
		delete cli;
		return 0;
	}
	if (cli->is_workflow_analyse() || cli->is_workflow_build() || cli->is_workflow_bundle() || cli->is_workflow_run()) {
		cli->find_corelib_and_toolchain();
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
