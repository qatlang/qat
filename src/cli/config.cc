#include "config.hpp"
#include "../cli/logger.hpp"
#include "../utils/qat_region.hpp"
#include "create.hpp"
#include "display.hpp"
#include "error.hpp"
#include "version.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/TargetParser/Triple.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <system_error>

#if PlatformIsMac
#include <mach-o/dyld.h>
#endif

#if defined _WIN32 || defined WIN32 || defined WIN64 || defined _WIN64
#include <SDKDDKVer.h>
#include <Windows.h>
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
// #define DISABLE_NEWLINE_AUTO_RETURN        0x0008
#endif

namespace qat::cli {

Config* Config::instance = nullptr;

Config const* Config::init(u64          count,
                           const char** args) { // NOLINT(modernize-avoid-c-arrays)
  if (!Config::instance) {
    return new Config(count, args);
  } else {
    return get();
  }
}

bool Config::hasInstance() { return Config::instance != nullptr; }

Config const* Config::get() { return Config::instance; }

Maybe<std::filesystem::path> Config::get_exe_path_from_env(String name) {
  auto hostTriple = llvm::Triple(LLVM_HOST_TRIPLE);
  auto pathEnv    = std::getenv(hostTriple.isOSWindows() ? "Path" : "PATH");
  if (pathEnv != nullptr) {
    auto  sep  = hostTriple.isOSWindows() ? ';' : ':';
    auto  path = String(pathEnv);
    usize i    = 0;
    while (path.find(sep, i) != String::npos) {
      auto ind       = path.find(sep, i);
      auto split     = path.substr(i, ind - i);
      auto splitPath = std::filesystem::path(split);
      if (std::filesystem::exists(splitPath / name)) {
        const auto entityPath = splitPath / name;
        if (std::filesystem::is_regular_file(entityPath)) {
          return entityPath;
        } else if (std::filesystem::is_symlink(entityPath)) {
          std::error_code err;
          auto            canonicalSplit = std::filesystem::canonical(entityPath, err);
          if (!err && std::filesystem::is_regular_file(canonicalSplit)) {
            return canonicalSplit;
          }
        }
      } else if (hostTriple.isOSWindows() && std::filesystem::exists(splitPath / (name + ".exe"))) {
        const auto entityPath = splitPath / (name + ".exe");
        if (std::filesystem::is_regular_file(entityPath)) {
          return entityPath;
        } else if (std::filesystem::is_symlink(entityPath)) {
          std::error_code err;
          auto            canonicalSplit = std::filesystem::canonical(entityPath, err);
          if (!err && std::filesystem::is_regular_file(canonicalSplit)) {
            return canonicalSplit;
          }
        }
      }
      i = ind + 1;
    }
  }
  return None;
}

Maybe<std::filesystem::path> Config::get_exe_path(String name) {
  auto hostTriple  = llvm::Triple(LLVM_HOST_TRIPLE);
  auto pathFromEnv = get_exe_path_from_env(name);
  if (pathFromEnv.has_value()) {
    return pathFromEnv;
  }
  std::error_code err;
  auto            resolvedPath = std::filesystem::canonical(hostTriple.isOSWindows() ? (name + ".exe") : name, err);
  if (err) {
#if PlatformIsWindows
    auto pipe = _popen((hostTriple.isOSWindows() ? ("where.exe " + name + ".exe") : ("which " + name)).c_str(), "r");

#else
    auto pipe = popen((hostTriple.isOSWindows() ? ("where.exe " + name + ".exe") : ("which " + name)).c_str(), "r");
#endif
    if (!pipe) {
      return None;
    }
    constexpr auto OUTPUT_BUFFER_LENGTH = 128;
    char           buffer[OUTPUT_BUFFER_LENGTH];
    String         output = "";
    while (!feof(pipe)) {
      if (fgets(buffer, OUTPUT_BUFFER_LENGTH, pipe) != nullptr) {
        output += buffer;
      }
    }
#if PlatformIsWindows
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    if (!output.empty()) {
      String result;
      if (output.find('\n') != String::npos) {
        result = output.substr(0, output.find('\n'));
      } else {
        result = output;
      }
      if (std::filesystem::exists(result)) {
        return result;
      }
    }
  } else {
    return resolvedPath;
  }
  return None;
}

void Config::setupEnvForQat() {
  auto& log        = Logger::get();
  auto  qatPathEnv = Config::get_exe_path_from_env("qat");
  if (!qatPathEnv.has_value()) {
    // QAT_PATH is not set
    auto qatPathRes = get_exe_path("qat");
    if (qatPathRes.has_value()) {
      qatDirPath = qatPathRes.value().parent_path();
    } else {
#if PlatformIsWindows
      char selfdir[MAX_PATH] = {0};
      auto charLen           = GetModuleFileNameA(NULL, selfdir, MAX_PATH);
      if (charLen > 0) {
        // PathRemoveFileSpecA(selfdir);
        qatDirPath = std::filesystem::path(String(selfdir, charLen));
      } else {
        log->fatalError("Path to qat could not be found", None);
      }
#elif PlatformIsMac
      char     path[1024] = {0};
      uint32_t size       = sizeof(path);
      if (_NSGetExecutablePath(path, &size) == 0) {
        qatDirPath = std::filesystem::path(String(path, size));
      } else {
        log->fatalError("Path to qat could not be found", None);
      }
#else
      std::error_code err;
      qatDirPath = std::filesystem::canonical("/proc/self/exe", err);
      if (err) {
        log->fatalError("Could not retrieve path to the qat executable", None);
      }
#endif
      qatDirPath = qatDirPath.parent_path();
      if (!std::filesystem::exists(qatDirPath)) {
        log->fatalError("Could not retrieve path to the qat executable", None);
      }
    }
#if PlatformIsWindows
    HKEY         hKey;
    LPCSTR       keyPath = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
    Vec<wchar_t> pathBuffer;
    pathBuffer.reserve(2048);
    DWORD pathBufferSize = pathBuffer.size();
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
      if (RegQueryValueEx(hKey, "Path", nullptr, nullptr, reinterpret_cast<LPBYTE>(pathBuffer.data()),
                          &pathBufferSize) != ERROR_SUCCESS) {
        log->fatalError(
            "Path could not be retrieved from the registry. Make sure that this executable is running in the administrator mode",
            None);
      }
      RegCloseKey(hKey);
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        pathBuffer.push_back(L';');
        for (auto w : qatDirPath.wstring()) {
          pathBuffer.push_back(w);
        }
        if (RegSetValueEx(hKey, "Path", 0, REG_EXPAND_SZ, reinterpret_cast<const BYTE*>(pathBuffer.data()),
                          wcslen(pathBuffer.data()) * sizeof(wchar_t)) != ERROR_SUCCESS) {
          log->fatalError(
              "Could not set `Path` in the registry. Make sure that this executable is running in administrator mode",
              None);
        }
        RegCloseKey(hKey);
      } else {
        log->fatalError(
            "Could not update registry key. Make sure that this executable is running in administrator mode", None);
      }
    } else {
      log->fatalError("Could not open registry key. Make sure that this executable is running in administrator mode",
                      None);
    }
    log->warn(
        "Path has been updated in the registry. Please close and reopen this terminal for the changes to take effect",
        None);
#else
    auto homeEnv = std::getenv("HOME");
    if (homeEnv == nullptr) {
      log->fatalError("Could not get HOME directory from the program environment", None);
    }
    auto           homePath       = fs::path(homeEnv);
    bool           foundBashFile  = false;
    bool           foundZshFile   = false;
    const fs::path bashConfigPath = homePath / ".bashrc";
    const fs::path zshConfigPath  = homePath / ".zshrc";
    if (fs::exists(bashConfigPath) && fs::is_regular_file(bashConfigPath)) {
      foundBashFile = true;
      std::ofstream outstream(homePath / ".bashrc", std::ios::app);
      outstream << "\nexport PATH=\"" << qatDirPath.string() << ":PATH\"\n";
      outstream.close();
    }
    if (fs::exists(zshConfigPath) && fs::is_regular_file(zshConfigPath)) {
      foundZshFile = true;
      std::ofstream outstream(homePath / ".zshrc", std::ios::app);
      outstream << "\nexport PATH=\"" << qatDirPath.string() << ":PATH\"\n";
      outstream.close();
    }
    if (!foundBashFile && !foundZshFile) {
      log->fatalError("Could not find .bashrc or .zshrc or similar configuration file for "
                      "your terminal session. Please add PATH=\"" +
                          qatDirPath.parent_path().string() + ":PATH\"" + " to the environment manually",
                      None);
    } else {
      log->warn("PATH has been updated in " + String(foundBashFile ? bashConfigPath.string() : "") +
                    (foundZshFile ? ((foundBashFile ? " and " : "") + zshConfigPath.string()) : "") +
                    ". Please close and reopen the terminal "
                    "for the session to retrieve the updated environment",
                None);
    }
#endif
  } else {
    qatDirPath = qatPathEnv.value().parent_path();
  }
  qatDirPath = fs::canonical(qatDirPath);
  // FIXME - Update this when nostdlib feature is added
  auto stdPathCand = qatDirPath / "../std";
  if (!isNoStd && fs::exists(stdPathCand)) {
    stdLibPath = stdPathCand / "std.lib.qat";
    if (!fs::exists(stdLibPath.value()) || !fs::is_regular_file(stdLibPath.value())) {
      if (!isNoStd) {
        log->fatalError("Found the standard library folder at path " + stdPathCand.string() +
                            " but could not find the library file in the folder. File " + stdLibPath.value().string() +
                            " is expected to be present",
                        None);
      } else {
        stdLibPath = None;
      }
    } else {
      stdLibPath = fs::canonical(stdLibPath.value());
    }
  } else if (!isNoStd) {
    log->fatalError("Could not find the standard library. Path to the qat executable was found to be " +
                        qatDirPath.string() + " but the standard library could not be found in " + stdPathCand.string(),
                    None);
  } else {
    stdLibPath = None;
  }
  auto toolchainCand = qatDirPath / "../toolchain";
  if (fs::exists(toolchainCand)) {
    if (fs::is_directory(toolchainCand)) {
      toolchainPath = toolchainCand;
    } else {
      log->fatalError(
          "The path " + toolchainCand.string() + " is not a directory. The " + log->color("toolchain") +
              " directory provided by the QAT installation is required for linking platform specific libraries used by QAT",
          None);
    }
  } else {
    log->fatalError(
        "Could not find the " + log->color("toolchain") +
            " directory in the QAT installation directory. This directory is required for linking platform specific dependencies required by QAT",
        None);
  }
}

String Config::filter_quotes(String value) {
  if (value.starts_with('"')) {
    value = value.substr(1);
    if (value.ends_with('"')) {
      value = value.substr(0, value.size() - 1);
    }
  }
  return value;
}

Config::Config(u64 count, const char** args)
    : exitAfter(false), verbose(false), saveDocs(false), showReport(false), exportAST(false), buildWorkflow(false),
      runWorkflow(false) {
  String verNum(VERSION_NUMBER);
  versionTuple = llvm::VersionTuple(
      std::atoi(verNum.substr(0, verNum.find_first_of('.')).c_str()),
      std::atoi(verNum.substr(verNum.find_first_of('.') + 1, verNum.find_last_of('.') - verNum.find_first_of('.') - 1)
                    .c_str()),
      std::atoi(verNum.substr(verNum.find_last_of('.') + 1, verNum.length() - verNum.find_last_of('.') - 1).c_str()));
  if (!hasInstance()) {
#if defined _WIN32 || defined WIN32 || defined WIN64 || defined _WIN64
    HANDLE handle      = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  consoleMode = 0;
    GetConsoleMode(handle, &consoleMode);
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    // consoleMode |= DISABLE_NEWLINE_AUTO_RETURN;
    SetConsoleMode(handle, consoleMode);
#endif
    auto& log        = Logger::get();
    Config::instance = this;
    invokePath       = args[0];
    exitAfter        = false;
    if (count <= 1) {
      cli::Error("No commands or arguments provided!", None);
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

    if ((std::getenv("COLORTERM") != NULL) && ((std::strcmp(std::getenv("COLORTERM"), "truecolor") == 0) ||
                                               (std::strcmp(std::getenv("COLORTERM"), "24bit") == 0))) {
      colorMode = ColorMode::truecolor;
    } else if ((std::getenv("TERM") != NULL) &&
               ((std::strcmp(std::getenv("TERM"), "iterm") == 0) ||
                (std::strcmp(std::getenv("TERM"), "linux-truecolor") == 0) ||
                (std::strcmp(std::getenv("TERM"), "gnome-truecolor") == 0) ||
                (std::strcmp(std::getenv("TERM"), "screen-truecolor") == 0) ||
                (std::strcmp(std::getenv("TERM"), "tmux-truecolor") == 0) ||
                (std::strcmp(std::getenv("TERM"), "xterm-truecolor") == 0) || (std::getenv("WT_SESSION") != NULL))) {
      colorMode = ColorMode::truecolor;
    } else if ((std::getenv("TERM") != NULL) && (std::strcmp(std::getenv("TERM"), "xterm-256color") &&
                                                 std::strcmp(std::getenv("TERM"), "tmux-256color") &&
                                                 std::strcmp(std::getenv("TERM"), "gnome-256color"))) {
      colorMode = ColorMode::color256;
    } else {
      colorMode = ColorMode::none;
    }

    String command(args[1]);
    u64    proceed  = 2;
    auto   readNext = [&]() -> Maybe<String> {
      if (proceed < count) {
        auto result = args[proceed];
        proceed++;
        return String(result);
      } else {
        return None;
      }
    };
    if (command == "analyse") {
      analyseWorkflow = true;
      if (count == 2) {
        paths.push_back(fs::current_path());
      }
    } else if (command == "run") {
      buildWorkflow = true;
      runWorkflow   = true;
      if (count == 2) {
        paths.push_back(fs::current_path());
      }
    } else if (command == "build") {
      buildWorkflow = true;
      if (count == 2) {
        paths.push_back(fs::current_path());
      }
    } else if (command == "new") {
      auto next = readNext();
      if (next.has_value()) {
        String        projectName = next.value();
        bool          isLib       = false;
        Maybe<String> candPath;
        Maybe<String> vcs;
        next = readNext();
        if (next.has_value()) {
          if (next.value() == "--lib") {
            isLib = true;
          } else if (next.value() == "--vcs") {
            if (vcs.has_value()) {
              log->fatalError("The '--vcs' argument has already been provided", None);
            }
            next = readNext();
            if (next.has_value()) {
              vcs = next;
            } else {
              log->fatalError("Expected a value for the '--vcs' argument. This argument determines"
                              " which version control system should be used for the new project."
                              " Use '--vcs=none' to disable this feature for this project",
                              None);
            }
          } else if (fs::exists(next.value())) {
            if (!fs::is_directory(next.value())) {
              log->fatalError("The provided path " + next.value() +
                                  " is not a directory and hence a project cannot be created in it",
                              next.value());
            }
            candPath = next;
          } else {
            log->fatalError("The provided path " + next.value() + " for creating the new project, does not exist",
                            next.value());
          }
          next = readNext();
          while (next.has_value()) {
            if (next.value() == "--lib") {
              if (isLib) {
                log->warn("The '--lib' option has already been provided", None);
              }
              isLib = true;
            } else if (next.value() == "--vcs") {
              if (vcs.has_value()) {
                log->fatalError("The '--vcs' argument has already been provided", None);
              }
              next = readNext();
              if (next.has_value()) {
                vcs = next;
              } else {
                log->fatalError("Expected a value for the '--vcs' argument. This argument determines"
                                " which version control system should be used for the new project."
                                " Use '--vcs=none' to disable this feature for this project",
                                None);
              }
            }
            next = readNext();
          }
        }
        cli::create_project(projectName, candPath.value_or(fs::current_path().string()), isLib,
                            vcs.has_value() ? ((vcs.value() == "none") ? None : vcs) : "git");
      } else {
        log->fatalError("The 'new' command expects the name of the project to be created", None);
      }
      exitAfter = true;
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
      auto next = readNext();
      if (next.has_value()) {
        String candidate = next.value();
        if (candidate == "build") {
          display::build_info(buildCommit);
        } else if (candidate == "web") {
          display::websites();
        }
        exitAfter = true;
      } else {
        cli::Error("Nothing to show here. Please provide name of the information to display. The available names are\n"
                   "build\nweb",
                   None);
      }
    }
    for (usize i = proceed; ((i < count) && !exitAfter); i++) {
      String arg     = args[i];
      auto   hasNext = [&]() { return (i + 1) < count; };
      auto   getNext = [&]() {
        i++;
        return String(args[i]);
      };
      if (arg == "-v" || arg == "--verbose") {
        verbose                 = true;
        Logger::get()->logLevel = LogLevel::VERBOSE;
      } else if (arg.find("--target=") == 0) {
        if (arg.length() > String::traits_type::length("--target=")) {
          targetTriple = filter_quotes(arg.substr(String::traits_type::length("--target=")));
        } else {
          cli::Error("Expected a valid path after --target=", None);
        }
      } else if (arg == "--target") {
        if (hasNext()) {
          targetTriple = filter_quotes(getNext());
        } else {
          log->fatalError("Expected a target triple string after " + log->color("--target"), None);
        }
      } else if (arg.find("--sysroot=") == 0) {
        if (arg.length() > String::traits_type::length("--sysroot=")) {
          sysRoot = filter_quotes(arg.substr(String::traits_type::length("--sysroot=")));
        } else {
          log->fatalError("Expected valid path for " + log->color("--sysroot"), None);
        }
      } else if (arg == "--sysroot") {
        if (hasNext()) {
          sysRoot = filter_quotes(getNext());
        } else {
          cli::Error("Expected a path for the sysroot after the --sysroot parameter", None);
        }
      } else if (arg.find("--clang=") == 0) {
        if (arg.length() > String::traits_type::length("--clang=")) {
          clangPath = arg.substr(String::traits_type::length("--clang="));
          if (!fs::exists(clangPath.value())) {
            cli::Error("Provided path for '--clang' does not exist: " + clangPath.value(), None);
          }
        } else {
          cli::Error("Expected valid path after --clang=", None);
        }
      } else if (String(arg) == "--clang") {
        if (hasNext()) {
          clangPath = (getNext());
          if (!fs::exists(clangPath.value())) {
            cli::Error("Provided path for '--clang' does not exist: " + clangPath.value(), None);
          }
        } else {
          cli::Error("Expected argument after '--clang' which would be the path to the clang executable to be used",
                     None);
        }
      } else if (arg.find("--linker=") == 0) {
        if (arg.length() > String::traits_type::length("--linker=")) {
          linkerPath = filter_quotes(arg.substr(String::traits_type::length("--linker=")));
          if (!fs::exists(linkerPath.value())) {
            cli::Error("Provided path to '--linker' does not exist: " + linkerPath.value(), None);
          }
        } else {
          cli::Error("Expected valid path after --linker=", None);
        }
      } else if (String(arg) == "--linker") {
        if (hasNext()) {
          linkerPath = filter_quotes(getNext());
          if (!fs::exists(linkerPath.value())) {
            cli::Error("Provided --linker path does not exist: " + linkerPath.value(), None);
          }
        } else {
          cli::Error("Expected argument after '--linker' which would be the path to the linker to be used", None);
        }
      } else if (arg == "--export-ast") {
        exportAST = true;
      } else if (arg == "--stats") {
        diagnostic = true;
      } else if (arg == "--export-code-info") {
        exportCodeInfo = true;
      } else if (arg == "-o" || arg == "--output") {
        if (hasNext()) {
          String out(filter_quotes(getNext()));
          if (fs::exists(out)) {
            if (fs::is_directory(out)) {
              outputPath = out;
            } else {
              cli::Error("Provided output path is not a directory! Please provide path to a directory", out);
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
            outputPath = out;
          }
        } else {
          cli::Error("Output path is not provided! Please provide path to a directory "
                     "for output, or don't mention the output argument at all so that the compiler chooses accordingly",
                     None);
        }
      } else if (arg == "--no-colors") {
        colorMode = ColorMode::none;
      } else if (arg == "--report") {
        showReport = true;
      } else if (arg == "--no-std") {
        stdLibPath = None;
        isNoStd    = true;
      } else if (arg == "--save-docs") {
        saveDocs = true;
      } else if (arg.starts_with("--mode=")) {
        auto modeVal = filter_quotes(arg.substr(String::traits_type::length("--mode=")));
        if (modeVal == "debug") {
          buildMode = BuildMode::debug;
        } else if (modeVal == "release") {
          buildMode = BuildMode::release;
        } else if (modeVal == "releaseWithDebugInfo") {
          buildMode = BuildMode::releaseWithDebugInfo;
        } else {
          log->fatalError("Invalid value for the argument " + log->color("--mode") + ". The possible values are " +
                              log->color("debug") + ", " + log->color("release") + " and " +
                              log->color("releaseWithDebugInfo"),
                          None);
        }
      } else if (arg == "--debug") {
        buildMode = BuildMode::debug;
      } else if (arg == "--release") {
        buildMode = BuildMode::release;
      } else if (arg == "--releaseWithDebugInfo") {
        buildMode = BuildMode::releaseWithDebugInfo;
      } else if (arg == "--static") {
        buildStatic = true;
      } else if (arg == "--shared") {
        buildShared = true;
      } else if (arg == "--clear-llvm") {
        clearLLVMFiles = true;
      } else {
        if (fs::exists(arg)) {
          paths.push_back(fs::path(arg));
        } else {
          if (arg.starts_with("--")) {
            log->fatalError(
                "Unrecognised argument " + log->color(arg) + " and it is also not an existing file or directory", arg);
          }
          log->fatalError(
              "Provided path " + log->color(arg) + " does not exist! Please provide path to a file or directory", arg);
        }
      }
    }
    if (exitAfter && ((count - 1) >= proceed)) {
      String otherArgs;
      for (usize i = proceed; i < count; i++) {
        otherArgs += args[proceed];
        if (i != (count - 1)) {
          otherArgs += ", ";
        }
      }
      log->warn("Ignoring additional arguments provided: " + otherArgs, None);
    }
    if (!outputPath.has_value() && !exitAfter) {
      if ((is_workflow_build() || is_workflow_bundle() || is_workflow_analyse() || is_workflow_run()) &&
          paths.size() == 1) {
        if ((fs::is_regular_file(paths[0]) && (paths[0].extension() == ".qat")) || fs::is_directory(paths[0])) {
          auto candParent = fs::is_directory(paths[0]) ? fs::path(paths[0]) : fs::path(paths[0]).parent_path();
          if (fs::exists(candParent)) {
            auto outDir = candParent / ".qatcache";
            outputPath  = outDir;
          } else {
            log->fatalError("The path " + log->color(candParent.string()) + " does not exist", candParent);
          }
        }
      }
      if (!outputPath.has_value()) {
        outputPath = fs::current_path() / ".qatcache";
      }
      if (!fs::exists(outputPath.value())) {
        std::error_code err;
        fs::create_directories(outputPath.value(), err);
        if (err) {
          log->fatalError("Could not create the directory " + outputPath.value().string() +
                              " for output. The error is: " + err.message(),
                          outputPath.value());
        }
      }
    }
    setupEnvForQat();
  }
}

} // namespace qat::cli