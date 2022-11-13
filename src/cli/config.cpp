#include "./config.hpp"
#include "./display.hpp"
#include "error.hpp"
#include "llvm/Config/llvm-config.h"
#include <filesystem>

namespace qat::cli {

Config* Config::instance = nullptr;

Config::~Config() {
  if (Config::instance) {
    delete Config::instance;
    Config::instance = nullptr;
  }
}

Config* Config::init(u64   count,
                     char* args[]) { // NOLINT(modernize-avoid-c-arrays)
  if (!Config::instance) {
    return new Config(count, args);
  } else {
    return get();
  }
}

void Config::destroy() {
  if (Config::instance) {
    auto* inst       = Config::instance;
    Config::instance = nullptr;
    delete inst;
  }
}

bool Config::isCompile() const { return (compile && !paths.empty()); }

bool Config::hasInstance() { return Config::instance != nullptr; }

bool Config::shouldExit() const { return exitAfter; }

Config* Config::get() { return Config::instance; }

Config::Config(u64 count, char** args)
    : exitAfter(false), verbose(false), saveDocs(false), showReport(false), export_ast(false), compile(false),
      run(false), releaseMode(false) {
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
      display::about(buildCommit);
      exitAfter = true;
    } else if (command == "version") {
      display::version();
      exitAfter = true;
    } else if (command == "show") {
      if (count == 2) {
        cli::Error("Nothing to show here. Please provide name of the information "
                   "to display",
                   None);
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
    } else if (command == "export-ast") {
      export_ast = true;
      if (count == 2) {
        paths.push_back(fs::current_path());
      }
    }
    for (usize i = proceed; ((i < count) && !exitAfter); i++) {
      String arg = args[i];
      if (arg == "-v" || arg == "--verbose") {
        verbose = true;
      } else if (String(arg).find("-t=") == 0) {
        targetTriple = String(arg).substr(3);
      } else if (String(arg).find("--target=") == 0) {
        targetTriple = String(arg).substr(9); // NOLINT(readability-magic-numbers)
      } else if (arg == "-t" || arg == "--target") {
        if ((i + 1) < count) {
          targetTriple = args[i + 1];
          i++;
        } else {
          cli::Error("Expected a target after " + arg + " flag", None);
        }
      } else if (String(arg).find("--sysroot=") == 0) {
        if (String(arg).length() > 10) {
          sysRoot = String(arg).substr(10);
        } else {
          cli::Error("Expected valid path for sysroot", None);
        }
      } else if (arg == "--sysroot") {
        if (i + 1 < count) {
          sysRoot = args[i + 1];
          i++;
        } else {
          cli::Error("Expected a path for the sysroot after the --sysroot parameter", None);
        }
      } else if (arg == "--wasm") {
        isWASM = true;
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
            cli::Error("Provided output path does not exist! Please provide "
                       "path to an existing directory",
                       out);
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

fs::path Config::getOutputPath() const { return outputPath.value(); }

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

bool Config::isWasmMode() const { return isWASM; }

bool Config::shouldBuildStatic() const { return buildShared.has_value() ? buildStatic.value_or(false) : true; }

bool Config::shouldBuildShared() const { return buildShared.value_or(false); }

} // namespace qat::cli