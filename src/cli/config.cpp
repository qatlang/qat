#include "./config.hpp"
#include "../show.hpp"
#include "../utils/unique_id.hpp"
#include "color.hpp"
#include "error.hpp"
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

CompileTarget Config::parseCompileTarget(const String& val) {
  if (val == "cpp") {
    return CompileTarget::cpp;
  } else if (val == "json") {
    return CompileTarget::json;
  } else {
    return CompileTarget::normal;
  }
}

Config::Config(u64 count, char** args)
    : target(CompileTarget::normal), exitAfter(false), verbose(false), saveDocs(false), showReport(false),
      export_ast(false), compile(false), run(false), outputInTemporaryPath(false), releaseMode(false) {
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
        } else if (candidate == "errors") {
          display::errors();
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
        target = parseCompileTarget(String(arg).substr(3));
      } else if (String(arg).find("--target=") == 0) {
        target = parseCompileTarget(String(arg).substr(9)); // NOLINT(readability-magic-numbers)
      } else if (arg == "-t" || arg == "--target") {
        if ((i + 1) < count) {
          target = parseCompileTarget(args[i + 1]);
        } else {
          cli::Error("Expected a target after " + arg + " flag", None);
        }
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
      } else if (arg == "--tmp-dir") {
        outputInTemporaryPath = true;
      } else if (arg == "--release") {
        releaseMode = true;
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
      if (outputInTemporaryPath) {
        outputPath = fs::temp_directory_path() / ".qatcache" / utils::unique_id();
        fs::create_directories(outputPath.value());
      } else {
        outputPath = fs::current_path();
      }
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

CompileTarget Config::getTarget() const { return target; }

bool Config::outputToTempDir() const { return outputInTemporaryPath; }

bool Config::noColorMode() const { return noColors; }

bool Config::isDebugMode() const { return !releaseMode; }

bool Config::isReleaseMode() const { return releaseMode; }

} // namespace qat::cli