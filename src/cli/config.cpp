#include "config.hpp"
#include "../memory_tracker.hpp"
#include "../show.hpp"
#include "display.hpp"
#include "error.hpp"
#include "version.hpp"
#include "llvm/Config/llvm-config.h"
#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace qat::cli {

Config* Config::instance = nullptr;

Config* Config::init(u64          count,
                     const char** args) { // NOLINT(modernize-avoid-c-arrays)
  if (!Config::instance) {
    return new Config(count, args);
  } else {
    return get();
  }
}

bool Config::isCompile() const { return (compile && !paths.empty()); }

bool Config::hasInstance() { return Config::instance != nullptr; }

bool Config::shouldExit() const { return exitAfter; }

bool Config::keepLLVM() const { return keepLLVMFiles; }

Config* Config::get() { return Config::instance; }

Config::Config(u64 count, const char** args)
    : exitAfter(false), verbose(false), saveDocs(false), showReport(false), export_ast(false), compile(false),
      run(false), releaseMode(false) {
  String verNum(VERSION_NUMBER);
  versionTuple = llvm::VersionTuple(
      std::atoi(verNum.substr(0, verNum.find_first_of('.')).c_str()),
      std::atoi(verNum.substr(verNum.find_first_of('.') + 1, verNum.find_last_of('.') - verNum.find_first_of('.') - 1)
                    .c_str()),
      std::atoi(verNum.substr(verNum.find_last_of('.') + 1, verNum.length() - verNum.find_last_of('.') - 1).c_str()));
  if (!hasInstance()) {
    Config::instance = this;
    invokePath       = args[0];
    exitAfter        = false;
    if (count <= 1) {
      cli::Error("Meow!! No commands or arguments provided", None);
    }
    buildCommit = BUILD_COMMIT_QUOTED;
    if (buildCommit.find('"') != String::npos) {
      String res;
      for (auto chr : buildCommit) {
        if (chr != '"') {
          res += chr;
        }
      }
      buildCommit = res;
    }

    String command(args[1]);
    u64    proceed = 2;
    if (command == "analyse") {
      analyse = true;
      if (count == 2) {
        paths.push_back(fs::current_path());
        return;
      }
    } else if (command == "run") {
      compile = true;
      run     = true;
      if (count == 2) {
        paths.push_back(fs::current_path());
        return;
      }
    } else if (command == "build") {
      compile = true;
      if (count == 2) {
        paths.push_back(fs::current_path());
        return;
      }
    } else if (command == "help") {
      display::help();
      exitAfter = true;
    } else if (command == "about") {
      display::about();
      exitAfter = true;
    } else if (command == "version") {
      display::detailedVersion(buildCommit);
      exitAfter = true;
    } else if (command == "--version") {
      display::shortVersion();
      exitAfter = true;
    } else if (command == "show") {
      if (count == 2) {
        cli::Error("Nothing to show here. Please provide name of the information to display", None);
      } else {
        String candidate = args[2];
        if (candidate == "build-info") {
          display::build_info(buildCommit);
        } else if (candidate == "web") {
          display::websites();
        }
        proceed   = 3;
        exitAfter = true;
      }
    }
    for (usize i = proceed; ((i < count) && !exitAfter); i++) {
      String arg = args[i];
      if (arg == "-v" || arg == "--verbose") {
        verbose = true;
      } else if (String(arg).find("--target=") == 0) {
        if (String(arg).length() > std::string::traits_type::length("--target=")) {
          targetTriple = String(arg).substr(std::string::traits_type::length("--target="));
        } else {
          cli::Error("Expected a valid path after --target=", None);
        }
      } else if (arg == "--target") {
        if ((i + 1) < count) {
          targetTriple = args[i + 1];
          i++;
        } else {
          cli::Error("Expected a target after --target", None);
        }
      } else if (String(arg).find("--sysroot=") == 0) {
        if (String(arg).length() > std::string::traits_type::length("--sysroot=")) {
          sysRoot = String(arg).substr(std::string::traits_type::length("--sysroot="));
        } else {
          cli::Error("Expected valid path for --sysroot", None);
        }
      } else if (arg == "--sysroot") {
        if (i + 1 < count) {
          sysRoot = args[i + 1];
          i++;
        } else {
          cli::Error("Expected a path for the sysroot after the --sysroot parameter", None);
        }
      } else if (String(arg).find("--clang-path=") == 0) {
        if (String(arg).length() > std::string::traits_type::length("--clang-path=")) {
          clangPath = String(arg.substr(std::string::traits_type::length("--clang-path=")));
        } else {
          cli::Error("Expected valid path for --clang-path", None);
        }
      } else if (String(arg) == "--clang-path") {
        if ((i + 1) < count) {
          clangPath = String(args[i + 1]);
          i++;
        } else {
          cli::Error("Expected argument after --clang-path which would be the path to the clang executable to use",
                     None);
        }
      } else if (arg == "--export-ast") {
        export_ast = true;
      } else if (arg == "--export-code-info") {
        exportCodeInfo = true;
      } else if (arg == "-o" || arg == "--output") {
        if ((i + 1) < count) {
          String out(args[i + 1]);
          if (fs::exists(out)) {
            if (fs::is_directory(out)) {
              outputPath = out;
            } else {
              cli::Error("Provided output path is not a directory! Please "
                         "provide path to a directory",
                         out);
            }
          } else {
            std::error_code errorCode;
            fs::create_directories(out, errorCode);
            if (errorCode) {
              cli::Error(
                  "Provided output path did not exist and creating the directory was unsuccessful with the error: " +
                      errorCode.message(),
                  out);
            }
          }
          i++;
        } else {
          cli::Error("Output path is not provided! Please provide path to a directory "
                     "for output, or don't mention the output flag at all.",
                     None);
        }
      } else if (arg == "--no-colors") {
        noColors = true;
      } else if (arg == "--report") {
        showReport = true;
      } else if (arg == "--save-docs") {
        saveDocs = true;
      } else if (arg == "--release") {
        releaseMode = true;
      } else if (arg == "--static") {
        buildStatic = true;
      } else if (arg == "--shared") {
        buildShared = true;
      } else if (arg == "--keep-llvm") {
        keepLLVMFiles = true;
      } else {
        if (fs::exists(arg)) {
          paths.push_back(fs::path(arg));
        } else {
          cli::Error("Provided path does not exist! Please provide path to a "
                     "file or directory",
                     arg);
        }
      }
    }
    if (exitAfter && ((count - 1) >= proceed)) {
      cli::Warning("Ignoring additional arguments provided...", None);
    }
    if (!outputPath.has_value()) {
      outputPath = fs::current_path() / ".qatcache";
    }
  }
}

Vec<fs::path> Config::getPaths() const { return paths; }

bool Config::shouldSaveDocs() const { return saveDocs; }

bool Config::hasOutputPath() const { return outputPath.has_value(); }

fs::path Config::getOutputPath() const { return outputPath.value_or(fs::current_path()); }

bool Config::shouldShowReport() const { return showReport; }

bool Config::shouldExportAST() const { return export_ast; }

bool Config::isVerbose() const { return verbose; }

bool Config::isRun() const { return run; }

bool Config::isAnalyse() const { return analyse; }

String Config::getTargetTriple() const { return targetTriple.value_or(LLVM_HOST_TRIPLE); }

bool Config::noColorMode() const { return noColors; }

bool Config::isDebugMode() const { return !releaseMode; }

bool Config::isReleaseMode() const { return releaseMode; }

bool Config::hasSysroot() const { return sysRoot.has_value(); }

String Config::getSysroot() const { return sysRoot.value_or(""); }

bool Config::shouldBuildStatic() const { return buildShared.has_value() ? buildStatic.value_or(false) : true; }

bool Config::shouldBuildShared() const { return buildShared.value_or(false); }

bool Config::exportCodeMetadata() const { return exportCodeInfo; }

const llvm::VersionTuple& Config::getVersionTuple() const { return versionTuple; }

} // namespace qat::cli