#include "./display.hpp"
#include "./version.hpp"

namespace qat::cli::display {

void version() { std::cout << VERSION_STRING << std::endl; }

void about(const String& buildCommit) {
  std::cout << "Version: " << VERSION_STRING << "\nCreator: "
            << "Aldrin Mathew"
            << " (https://github.com/aldrinmathew)"
            << "\nBuild Commit: " << buildCommit << "\nBuild Branch: " << BUILD_BRANCH << "\nBuild Type: " << BUILD_TYPE
            << "\nWebsite: https://qat.dev" << std::endl;
}

void build_info(const String& buildCommit) {
  std::cout << "Build Info\n"
               "  Version: "
            << VERSION_STRING << "+" << buildCommit << "\n  Commit: " << buildCommit << "\n  Branch: " << BUILD_BRANCH
            << std::endl;
}

void help() {}

void websites() {
  std::cout << "Website           :: https://qat.dev\n"
            << "Docs              :: https://docs.qat.dev\n"
            << "Qat Repositories  :: https://github.com/qatlang\n"
            << "Aldrin Mathew\n"
            << "        (Github)  :: https://github.com/aldrinmathew\n"
            << "        (Gitlab)  :: https://gitlab.com/aldrinmathew\n"
            << "        (Youtube) :: https://youtube.com/@aldrinmathew" << std::endl;
}

} // namespace qat::cli::display