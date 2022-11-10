#include "./display.hpp"

namespace qat::cli::display {

void version() { std::cout << "qat " << colors::bold::green << "v" << VERSION_STRING << colors::reset << std::endl; }

void about(String buildCommit) {
  std::cout << "Version: " << colors::bold::green << VERSION_STRING << colors::reset
            << "\nCreator: " << colors::bold::green << "Aldrin Mathew" << colors::reset
            << " (https://github.com/aldrinmathew)"
            << "\nBuild Commit: " << buildCommit << "\nBuild Branch: " << BUILD_BRANCH << "\nBuild Type: " << BUILD_TYPE
            << "\nLicense: Inspectable License 1.0"
            << "\nWebsite: https://qat.dev" << colors::reset << std::endl;
}

void build_info(String buildCommit) {
  std::cout << "Build Info\n  Version: " << colors::bold::green << VERSION_STRING << "+" << buildCommit << colors::reset
            << "\n  Commit: " << colors::bold::green << buildCommit << colors::reset
            << "\n  Branch: " << colors::bold::green << BUILD_BRANCH << colors::reset << std::endl;
}

void help() {}

void websites() {
  std::cout << "Website                 :: https://qat.dev\n"
            << "Docs                    :: https://docs.qat.dev\n"
            << "Qat Org                 :: https://github.com/qatlang\n"
            << "Aldrin Mathew (Github)  :: https://github.com/aldrinmathew\n"
            << "              (Gitlab)  :: https://gitlab.com/aldrinmathew\n"
            << "              (Youtube) :: https://youtube.com/@aldrinmathew" << std::endl;
}

} // namespace qat::cli::display