#include "config.hpp"
#include "../memory_tracker.hpp"
#include "../show.hpp"
#include "../utils/logger.hpp"
#include "display.hpp"
#include "error.hpp"
#include "version.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/TargetParser/Triple.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
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

bool Config::hasInstance() { return Config::instance != nullptr; }

Config* Config::get() { return Config::instance; }

Maybe<std::filesystem::path> Config::getExePathFromEnvPath(String name) {
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

Maybe<std::filesystem::path> Config::getExePath(String name) {
  auto hostTriple  = llvm::Triple(LLVM_HOST_TRIPLE);
  auto pathFromEnv = getExePathFromEnvPath(name);
  if (pathFromEnv.has_value()) {
    return pathFromEnv;
  }
  std::error_code err;
  auto            resolvedPath = std::filesystem::canonical(hostTriple.isOSWindows() ? (name + ".exe") : name, err);
  if (err) {
    auto pipe = popen((hostTriple.isOSWindows() ? ("where.exe " + name + ".exe") : ("which " + name)).c_str(), "r");
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
    pclose(pipe);
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
  auto  qatPathEnv = Config::getExePathFromEnvPath("qat");
  if (!qatPathEnv.has_value()) {
    log->setPersistent("Configuring environment for qat");
    log->enableBuffering();
    // QAT_PATH is not set
    auto qatPathRes = getExePath("qat");
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
        log->error("Path to qat could not be found", None);
      }
#elif PlatformIsMac
      char     path[1024] = {0};
      uint32_t size       = sizeof(path);
      if (_NSGetExecutablePath(path, &size) == 0) {
        qatDirPath = std::filesystem::path(String(path, size));
      } else {
        log->error("Path to qat could not be found", None);
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
    HKEY           hKey;
    const wchar_t* keyPath = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
    Vec<wchar_t>   pathBuffer;
    pathBuffer.reserve(2048);
    DWORD pathBufferSize = pathBuffer.size();
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
      if (RegQueryValueEx(hKey, L"Path", nullptr, nullptr, reinterpret_cast<LPBYTE>(pathBuffer.data()),
                          &pathBufferSize) != ERROR_SUCCESS) {
        log->error(
            "Path could not be retrieved from the registry. Make sure that this executable is running in the administrator mode",
            None);
      }
      RegCloseKey(hKey);
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        pathBuffer += L";" + std::wstring(qatPath.string());
        if (RegSetValueEx(hKey, L"Path", 0, REG_EXPAND_SZ, reinterpret_cast<const BYTE*>(newValue),
                          wcslen(newValue) * sizeof(wchar_t)) != ERROR_SUCCESS) {
          log->error("Could not set " + colored("Path") +
                         " in the registry. Make sure that this executable is running in administrator mode",
                     None);
        }
        RegCloseKey(hKey);
      } else {
        log->error("Could not update registry key. Make sure that this executable is running in administrator mode",
                   None);
      }
    } else {
      log->error("Could not open registry key. Make sure that this executable is running in administrator mode", None);
    }
    log->say(
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
      log->say("PATH has been updated in " + String(foundBashFile ? bashConfigPath.string() : "") +
               (foundZshFile ? ((foundBashFile ? " and " : "") + zshConfigPath.string()) : "") +
               ". Please close and reopen the terminal "
               "for the session to retrieve the updated environment");
    }
#endif
    log->disableBuffering();
    log->resetPersistent(true);
  } else {
    qatDirPath = qatPathEnv.value().parent_path();
  }
  qatDirPath = fs::canonical(qatDirPath);
  // FIXME - Update this when nostdlib feature is added
  auto stdPathCand = qatDirPath / "../std";
  if (fs::exists(stdPathCand)) {
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
}

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

    String command(args[1]);
    u64    proceed = 2;
    if (command == "analyse") {
      analyse = true;
      if (count == 2) {
        paths.push_back(fs::current_path());
      }
    } else if (command == "run") {
      compile = true;
      run     = true;
      if (count == 2) {
        paths.push_back(fs::current_path());
      }
    } else if (command == "build") {
      compile = true;
      if (count == 2) {
        paths.push_back(fs::current_path());
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
        if (String(arg).length() > String::traits_type::length("--sysroot=")) {
          sysRoot = String(arg).substr(String::traits_type::length("--sysroot="));
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
      } else if (String(arg).find("--clang=") == 0) {
        if (String(arg).length() > String::traits_type::length("--clang=")) {
          clangPath = String(arg.substr(String::traits_type::length("--clang=")));
          if (!fs::exists(clangPath.value())) {
            cli::Error("Provided path for --clang does not exist: " + clangPath.value(), None);
          }
        } else {
          cli::Error("Expected valid path after --clang=", None);
        }
      } else if (String(arg) == "--clang") {
        if ((i + 1) < count) {
          clangPath = String(args[i + 1]);
          if (!fs::exists(clangPath.value())) {
            cli::Error("Provided path for --clang does not exist: " + clangPath.value(), None);
          }
          i++;
        } else {
          cli::Error("Expected argument after --clang which would be the path to the clang executable to be used",
                     None);
        }
      } else if (String(arg).find("--linker=") == 0) {
        if (String(arg).length() > String::traits_type::length("--linker=")) {
          linkerPath = String(arg.substr(String::traits_type::length("--linker=")));
          if (!fs::exists(linkerPath.value())) {
            cli::Error("Provided --linker path does not exist: " + linkerPath.value(), None);
          }
        } else {
          cli::Error("Expected valid path after --linker=", None);
        }
      } else if (String(arg) == "--linker") {
        if ((i + 1) < count) {
          linkerPath = String(args[i + 1]);
          if (!fs::exists(linkerPath.value())) {
            cli::Error("Provided --linker path does not exist: " + linkerPath.value(), None);
          }
          i++;
        } else {
          cli::Error("Expected argument after --linker which would be the path to the linker to be used", None);
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
      } else if (arg == "--nostd") {
        stdLibPath = None;
        isNoStd    = true;
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
        if (fs::exists(fs::current_path() / arg)) {
          paths.push_back(fs::path(arg));
        } else {
          log->fatalError("Provided path does not exist! Please provide path to a file or directory", arg);
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
    if (!outputPath.has_value()) {
      outputPath = fs::current_path() / ".qatcache";
    }
    setupEnvForQat();
  }
}

} // namespace qat::cli