#include "./config.hpp"
#include "../show.hpp"
#include "error.hpp"

namespace qat::cli {

Config *Config::instance = nullptr;

Config::~Config() {
  if (Config::instance) {
    delete Config::instance;
    Config::instance = nullptr;
  }
}

Config *Config::init(u64 count, char *args[]) {
  if (!Config::instance) {
    return new Config(count, args);
  } else {
    return get();
  }
}

void Config::destroy() {
  if (Config::instance) {
    auto inst        = Config::instance;
    Config::instance = nullptr;
    delete inst;
  }
}

bool Config::isCompile() const { return (compile && !paths.empty()); }

bool Config::hasInstance() { return Config::instance != nullptr; }

bool Config::shouldExit() const { return exitAfter; }

Config *Config::get() { return Config::instance; }

CompileTarget Config::parseCompileTarget(const String &val) {
  if (val == "cpp") {
    return CompileTarget::cpp;
  } else if (val == "json") {
    return CompileTarget::json;
  } else {
    return CompileTarget::normal;
  }
}

Config::Config(u64 count, char **args)
    : exitAfter(false), verbose(false), saveDocs(false), showReport(false),
      lexer_emit_tokens(false), export_ast(false), compile(false) {
  target = CompileTarget::normal;
  if (!hasInstance()) {
    Config::instance = this;
    invokePath       = args[0];
    exitAfter        = false;
    if (count <= 1) {
      std::cout << "No commands or arguments provided" << std::endl;
      exit(0);
    }
    buildCommit = BUILD_COMMIT_QUOTED;
    if (buildCommit.find('"') != String::npos) {
      String res;
      for (auto ch : buildCommit) {
        if (ch != '"') {
          res += ch;
        }
      }
      buildCommit = res;
    }

    String command(args[1]);
    u64    proceed = 2;
    if (command == "run") {
      // TODO - Implement this after implementing Compile and Run
    } else if (command == "compile") {
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
        std::cout << "Nothing to show here" << std::endl;
      } else {
        String candidate = args[2];
        if (candidate == "build-info") {
          display::build_info(buildCommit);
        } else if (candidate == "web") {
          display::websites();
        } else if (candidate == "errors") {
          display::errors();
        }
        proceed = 3;
      }
      exitAfter = true;
    } else if (command == "export-ast") {
      export_ast = true;
      if (count == 2) {
        paths.push_back(fs::current_path());
      }
    }
    for (usize i = proceed; i < count; i++) {
      String arg = args[i];
      if (arg == "-v" || arg == "--verbose") {
        verbose = true;
      } else if (String(arg).find("-t=") == 0) {
        target = parseCompileTarget(String(arg).substr(3));
      } else if (String(arg).find("--target=") == 0) {
        target = parseCompileTarget(String(arg).substr(9));
      } else if (arg == "-t" || arg == "--target") {
        if ((i + 1) < count) {
          target = parseCompileTarget(args[i + 1]);
        } else {
          throw_error("Expected a target after " + arg + " flag", invokePath);
        }
      } else if (arg == "-o" || arg == "--output") {
        if ((i + 1) < count) {
          String out(args[i + 1]);
          if (fs::exists(out)) {
            if (fs::is_directory(out)) {
              outputPath = out;
            } else {
              std::cout
                  << "Provided output path " << HLIGHT(yellow, out)
                  << " is not a directory! Please provide path to a directory"
                  << std::endl;
              exitAfter = true;
            }
          } else {
            std::cout << "Provided output path " << HLIGHT(yellow, out)
                      << " does not exist! Please provide path to a directory"
                      << std::endl;
            exitAfter = true;
          }
          i++;
        } else {
          std::cout << "Output path is not provided! Please provide path to a "
                       "directory for output"
                    << std::endl;
          exitAfter = true;
        }
      } else if (arg == "--report") {
        showReport = true;
      } else if (arg == "--emit-tokens") {
        lexer_emit_tokens = true;
      } else if (arg == "--save-docs") {
        saveDocs = true;
      } else {
        if (fs::exists(arg)) {
          paths.push_back(fs::path(arg));
        } else {
          std::cout
              << "Provided path " << fs::path(arg).string()
              << " does not exist! Please provide path to a file or directory"
              << std::endl;
          exitAfter = true;
        }
      }
    }
    if (exitAfter && (proceed == 2) && (count > 2)) {
      std::cout << "\nIgnoring additional arguments provided...\n";
    }
  }
}

Vec<fs::path> Config::getPaths() const { return paths; }

bool Config::shouldSaveDocs() const { return saveDocs; }

bool Config::hasOutputPath() const { return outputPath.has_value(); }

fs::path Config::getOutputPath() const { return outputPath.value(); }

bool Config::shouldShowReport() const { return showReport; }

bool Config::shouldExportAST() const { return export_ast; }

bool Config::shouldLexerEmitTokens() const { return lexer_emit_tokens; }

bool Config::isVerbose() const { return verbose; }

CompileTarget Config::getTarget() const { return target; }

} // namespace qat::cli