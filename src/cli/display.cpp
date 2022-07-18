#include "./display.hpp"

namespace qat::cli::display {

void version() {
  std::cout << "qat " << colors::bold::green << "v" << VERSION_STRING
            << colors::reset << std::endl;
}

void about(std::string buildCommit) {
  std::cout << "Version: " << colors::bold::green << VERSION_STRING
            << colors::reset << "\nCreator: " << colors::bold::green
            << "Aldrin Mathew" << colors::reset
            << " (https://github.com/aldrinmathew)"
            << "\nBuild Commit: " << buildCommit
            << "\nBuild Branch: " << BUILD_BRANCH
            << "\nBuild Type: " << BUILD_TYPE
            << "\nLicense: Inspectable License 1.0"
            << "\nWebsite: https://qat.dev" << colors::reset << std::endl;
}

void build_info(std::string buildCommit) {
  std::cout << "Build Info\n  Version: " << colors::bold::green
            << VERSION_STRING << "+" << buildCommit << colors::reset
            << "\n  Commit: " << colors::bold::green << buildCommit
            << colors::reset << "\n  Branch: " << colors::bold::green
            << BUILD_BRANCH << colors::reset << std::endl;
}

void help() {}

void websites() {
  std::cout << "Website: https://qat.dev\n"
            << "Error Box: <Work in Progress>\n"
            << "Aldrin Mathew on Github: https://github.com/AldrinMathew"
            << std::endl;
}

void errors() { std::cout << "Work in progress..." << std::endl; }

} // namespace qat::cli::display