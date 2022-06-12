#include "./config.hpp"

namespace qat {
namespace CLI {

Config *Config::instance = nullptr;

Config::~Config() {
  if ((users - 1) >= 0) {
    users--;
  }
  if (users == 0 && Config::instance) {
    delete Config::instance;
    Config::instance = nullptr;
  }
};

Config Config::init(u64 count, const char **args) {
  if (instance == nullptr) {
    auto result = *(new Config(count, args));
    result.users++;
    return result;
  } else {
    return get();
  }
}

void Config::destroy() {
  if (Config::instance != nullptr) {
    delete Config::instance;
    Config::instance = nullptr;
  }
}

bool Config::has_instance() { return Config::instance != nullptr; }

Config Config::get() {
  if (has_instance()) {
    (*instance).users++;
    return *instance;
  }
}

Config::Config(u64 count, const char **args) {
  if (!has_instance()) {
    Config::instance = this;
    invoke_path = args[0];
    verbose = false;
    exit_after = false;
    if (count <= 1) {
      std::cout << "No commands or arguments provided" << std::endl;
      exit(0);
    }
    build_commit = BUILD_COMMIT_QUOTED;
    if (build_commit.find('"') != std::string::npos) {
      std::string res;
      for (auto ch : build_commit) {
        if (ch != '"') {
          res += ch;
        }
      }
      build_commit = res;
    }

    std::string command = args[1];
    int proceed = 2;
    if (command == "run") {
      // TODO - Implement this after implementing Compile and Run
    } else if (command == "build") {
      if (count == 2) {
        paths.push_back(fsexp::current_path());
        return;
      }
    } else if (command == "help") {
      display::help();
      exit_after = true;
    } else if (command == "about") {
      display::about(build_commit);
      exit_after = true;
    } else if (command == "version") {
      display::version();
      exit_after = true;
    } else if (command == "show") {
      if (count == 2) {
        std::cout << "Nothing to show here" << std::endl;
      } else {
        std::string candidate = args[2];
        if (candidate == "build-info") {
          display::build_info(build_commit);
        } else if (candidate == "web") {
          display::websites();
        } else if (candidate == "errors") {
          display::errors();
        }
        proceed = 3;
      }
      exit_after = true;
    }
    for (std::size_t i = proceed; i < count; i++) {
      std::string arg = args[i];
      if (arg == "-v" || arg == "--verbose") {
        verbose = true;
      } else if (arg == "--report") {
        show_report = true;
        // TODO - Set parser report option here once that is implemented
      } else if (arg == "--emit-tokens") {
        lexer_emit_tokens = true;
      } else if (arg == "--save-docs") {
        save_docs = true;
      } else {
        if (fsexp::exists(arg)) {
          paths.push_back(fsexp::path(arg));
        } else {
          std::cout
              << "Provided path " << fsexp::path(arg).string()
              << " does not exist! Please provide path to a file or directory"
              << std::endl;
          exit_after = true;
        }
      }
    }
    if (exit_after && (proceed == 2) && (count > 2)) {
      std::cout << "\nIgnoring additional arguments provided...\n";
    }
  }
}

} // namespace CLI
} // namespace qat