#include "./display.hpp"
#include "./version.hpp"

#include <llvm/Config/llvm-config.h>

namespace qat::cli::display {

void detailedVersion(String const& buildCommit) {
	std::cout << "QAT Compiler " << VERSION_STRING << "\n"
	          << "Target: " << LLVM_DEFAULT_TARGET_TRIPLE << "\n"
	          << ((String(LLVM_HOST_TRIPLE) != LLVM_DEFAULT_TARGET_TRIPLE) ? "Host: " LLVM_HOST_TRIPLE "\n" : "")
	          << "Build Type: " << BUILD_TYPE << "\n"
	          << "Build Branch: " << BUILD_BRANCH << "\n"
	          << "Build Commit: " << buildCommit << (QAT_GIT_HAS_CHANGES ? " (with modifications)" : "") << std::endl;
}

void shortVersion() { std::cout << VERSION_STRING << std::endl; }

void about() {
	std::cout << "The QAT Programming Language\n"
	          << "   Closer to your machine's heart...\n"
	          << "Created with ♥  by Aldrin Mathew (https://github.com/aldrinmathew)\n"
	          << "Visit https://qatlang.org for more details" << std::endl;
}

void build_info(const String& buildCommit) {
	std::cout << "Build Info\n"
	          << "  Version: " << VERSION_STRING << "\n"
	          << "  Type: " << BUILD_TYPE << "\n"
	          << "  Commit: " << buildCommit << "\n"
	          << "  Branch: " << BUILD_BRANCH << std::endl;
}

void help() {}

void websites() {
	std::cout << "Website           :: https://qatlang.org\n"
	          << "Docs              :: https://qatlang.org/docs\n"
	          << "Repositories      :: https://github.com/qatlang\n"
	          << "Aldrin Mathew\n"
	          << "        (Github)  :: https://github.com/aldrinmathew\n"
	          << "        (Gitlab)  :: https://gitlab.com/aldrinmathew\n"
	          << "        (Youtube) :: https://youtube.com/@aldrinmathew" << std::endl;
}

void target_triplets() {
	std::cout << "x86_64-unknown-linux-gnu\n"
	             "x86_64-pc-linux-gnu\n"
	             "x86_64-pc-windows-msvc\n"
	             "aarch64-unknown-linux-gnu\n"
	             "aarch64-pc-linux-gnu\n"
	             "amd64-apple-darwin\n"
	             "arm64-apple-darwin\n";
}

} // namespace qat::cli::display
