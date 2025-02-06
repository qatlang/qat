#include "./config.hpp"
#include "../cli/logger.hpp"
#include "../show.hpp"
#include "../utils/find_executable.hpp"
#include "./create.hpp"
#include "./display.hpp"
#include "./error.hpp"
#include "./version.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <system_error>

#if OS_IS_MAC
#include <mach-o/dyld.h>

#elif OS_IS_WINDOWS
#if RUNTIME_IS_MINGW
#include <sdkddkver.h>
#include <windows.h>
#elif RUNTIME_IS_MSVC
#include <SDKDDKVer.h>
#include <Windows.h>
#endif
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#define DISABLE_NEWLINE_AUTO_RETURN        0x0008

#endif // OS_IS_WINDOWS

namespace qat::cli {

Config* Config::instance = nullptr;

Config* Config::init(u64 count, const char** args) {
	if (not Config::instance) {
		return new Config(count, args);
	} else {
		return instance;
	}
}

bool Config::hasInstance() { return Config::instance != nullptr; }

Config const* Config::get() { return Config::instance; }

void Config::find_stdlib_and_toolchain() {
	auto& log = Logger::get();

	const auto qatPathEnv = find_executable("qat");
	if (not qatPathEnv.has_value()) {
		SHOW("Could not find qat path from env");
		return;
	}
	qatDirPath       = fs::path(qatPathEnv.value()).parent_path();
	auto stdPathCand = fs::absolute(qatDirPath / "../std");
	if (not isNoStd && not isFreestanding) {
		if (not fs::exists(stdPathCand)) {
			log->fatalError(
			    "Could not find the standard library. Path to the qat executable was found to be " +
			        qatDirPath.string() + " but the standard library could not be found in " + stdPathCand.string() +
			        ". If you wish to compile a project without the standard library, use the " +
			        log->color("--no-std") +
			        " flag. And if you wish to compile for a freestanding environment, use the " +
			        log->color("--freestanding") + " flag, which automatically implies " + log->color("--no-std") + ".",
			    None);
		}
		stdLibPath = stdPathCand / "std.lib.qat";
		if (not fs::exists(stdLibPath.value()) || not fs::is_regular_file(stdLibPath.value())) {
			log->fatalError("Found the standard library folder at path " + stdPathCand.string() +
			                    " but could not find the library file in the folder. The file " +
			                    log->color(stdLibPath.value().string()) + " is expected to be present",
			                None);
		}
		stdLibPath = fs::absolute(stdLibPath.value());
	} else {
		stdLibPath = None;
	}
	auto toolchainCand = qatDirPath / "../toolchain";
	if (not fs::exists(toolchainCand)) {
		log->fatalError(
		    "Could not find the " + log->color("toolchain") +
		        " directory in the qat installation directory. This directory is required for linking platform specific dependencies required by the language",
		    None);
	}
	if (not fs::is_directory(toolchainCand)) {
		log->fatalError(
		    "The path " + toolchainCand.string() + " is not a directory. The " + log->color("toolchain") +
		        " directory provided by the qat installation is required for linking platform specific dependencies required by the language",
		    None);
	}
	toolchainPath = fs::absolute(toolchainCand);
	log->say("qat is present in   := " + qatPathEnv.value());
	log->say("Standard Library    := " + (stdLibPath.has_value() ? stdLibPath.value().string() : "(Not Found)"));
	log->say("Toolchain Directory := " + (toolchainPath.has_value() ? toolchainPath.value().string() : "(Not Found)"));
}

void Config::setup_path_in_env(bool isSetupCmd) {
	auto&      log                = Logger::get();
	auto       qatPathEnv         = find_executable("qat");
	const auto foundInPathAlready = qatPathEnv.has_value();
	if (not foundInPathAlready) {
		// QAT_PATH is not set
#if OS_IS_WINDOWS
		char selfdir[MAX_PATH] = {0};
		auto charLen           = GetModuleFileNameA(NULL, selfdir, MAX_PATH);
		if (charLen > 0) {
			// PathRemoveFileSpecA(selfdir);
			qatDirPath = std::filesystem::path(String(selfdir, charLen));
		} else {
			log->fatalError("Path to qat could not be found", None);
		}
#elif OS_IS_MAC
		char     path[1024] = {0};
		uint32_t size       = sizeof(path);
		if (_NSGetExecutablePath(path, &size) == 0) {
			qatDirPath = std::filesystem::path(String(path, size));
		} else {
			log->fatalError("Path to qat could not be found", None);
		}
#elif OS_IS_LINUX || OS_IS_FREEBSD || OS_IS_HAIKU
		std::error_code err;
		qatDirPath = std::filesystem::canonical("/proc/self/exe", err);
		if (err) {
			log->fatalError("Could not retrieve path to the qat executable", None);
		}
#else
#error "Unsupported OS"
#endif
		qatDirPath = qatDirPath.parent_path();
		if (not std::filesystem::exists(qatDirPath)) {
			log->fatalError("Could not retrieve path to the " + log->color("qat") +
			                    " executable. Please add the directory of the " + log->color("qat") + " to the " +
			                    (OS_IS_WINDOWS ? ("system " + log->color("Path")) : "PATH") +
			                    " environment variable manually",
			                None);
		}
#if OS_IS_WINDOWS
		auto isProcessAdmin = []() -> BOOL {
			BOOL   result = false;
			HANDLE hToken = NULL;
			if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
				TOKEN_ELEVATION elevation;
				DWORD           dwSize;
				if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
					result = elevation.TokenIsElevated;
				}
			}
			if (hToken) {
				CloseHandle(hToken);
				hToken = NULL;
			}
			return result;
		};
		// I am not confident about the above function to check admin mode, so I am not removing errors about failures
		// related to the admin mode
		if (isProcessAdmin()) {
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
					    "Could not update registry key. Make sure that this executable is running in administrator mode",
					    None);
				}
			} else {
				log->fatalError(
				    "Could not open registry key. Make sure that this executable is running in administrator mode",
				    None);
			}
			log->warn(
			    "Path has been updated in the registry. Please close and reopen this terminal for the changes to take effect",
			    None);
		} else {
			log->fatalError(
			    "Please re-run this program in " + log->color("administrator mode") + ". The system " +
			        log->color("Path") +
			        " environment variable cannot be updated otherwise, to include path to the qat executable."
			        " If you wish to not run this program in administrator mode, please add path to the qat executable to the system " +
			        log->color("Path") + " environment variable manually. The qat executable was found in " +
			        log->color(qatDirPath.string()),
			    None);
		}
#else
		auto     homeEnv = std::getenv("HOME");
		fs::path homePath;
		if (homeEnv == nullptr) {
			log->warn("Could not get the " + log->color("HOME") +
			              " variable from the program environment, while trying to update the " + log->color("PATH") +
			              " environment variable. Using ~ instead to refer to the home directory",
			          None);
			homePath = "~";
		} else {
			homePath = fs::path(homeEnv);
		}
		bool           foundBashFile  = false;
		bool           foundZshFile   = false;
		const fs::path bashConfigPath = homePath / ".bashrc";
		const fs::path zshConfigPath  = homePath / ".zshrc";
#define QAT_PATH_PREPEND "export PATH=\"" + qatDirPath.string() + ":$PATH\""
#define QAT_PATH_APPEND  "export PATH=\"$PATH:" + qatDirPath.string() + '"'
		if (fs::exists(bashConfigPath) && fs::is_regular_file(bashConfigPath)) {
			foundBashFile = true;
			std::ifstream inStr(homePath / ".bashrc");
			bool          foundInPath = false;
			String        fileStr;
			while (std::getline(inStr, fileStr)) {
				if ((fileStr.find(QAT_PATH_PREPEND) != String::npos) ||
				    (fileStr.find(QAT_PATH_APPEND) != String::npos)) {
					foundInPath = true;
					break;
				}
			}
			inStr.close();
			if (not foundInPath) {
				std::ofstream outstream(homePath / ".bashrc", std::ios::app);
				outstream << '\n' << QAT_PATH_PREPEND << '\n';
				outstream.close();
			} else {
				log->warn("Detected that the path to the qat executable is already present in the file " +
				              log->color((homePath / ".bashrc").string()) + ". If your current shell is " +
				              log->color("bash") + " make sure to run " +
				              log->color("source " + (homePath / ".bashrc").string()) + " or " +
				              log->color("source ~/.bashrc") + " to source the contents of the configuration file",
				          None);
			}
		}
		if (fs::exists(zshConfigPath) && fs::is_regular_file(zshConfigPath)) {
			foundZshFile = true;
			std::ifstream inStr(homePath / ".zshrc");
			bool          foundInPath = false;
			String        fileStr;
			while (std::getline(inStr, fileStr)) {
				if ((fileStr.find(QAT_PATH_PREPEND) != String::npos) ||
				    (fileStr.find(QAT_PATH_APPEND) != String::npos)) {
					foundInPath = true;
					break;
				}
			}
			inStr.close();
			if (not foundInPath) {
				std::ofstream outstream(homePath / ".zshrc", std::ios::app);
				outstream << '\n' << QAT_PATH_PREPEND << '\n';
				outstream.close();
			} else {
				log->warn("Detected that the path to the qat executable is already present in the file " +
				              log->color((homePath / ".zshrc").string()) + ". If your current shell is " +
				              log->color("zsh") + " make sure to run " + log->color("source ~/.zshrc") + " or " +
				              log->color("source " + (homePath / ".zshrc").string()) +
				              " to source the contents of the configuration file",
				          None);
			}
		}
		if (not foundBashFile && not foundZshFile) {
			log->fatalError("Could not find .bashrc or .zshrc or similar configuration file for "
			                "your terminal session. Please add " +
			                    log->color("export PATH=\"" + qatDirPath.parent_path().string() + ":$PATH\"") +
			                    " to the environment manually",
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
		qatDirPath = fs::path(qatPathEnv.value()).parent_path();
	}
	qatDirPath = fs::absolute(qatDirPath);
	find_stdlib_and_toolchain();
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
		if (count < 2) {
			setup_path_in_env(false);
			exitAfter = true;
			return;
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

		auto termENV    = std::getenv("TERM");
		auto colTermENV = std::getenv("COLORTERM");
		if ((colTermENV != NULL) &&
		    ((std::strcmp(colTermENV, "truecolor") == 0) || (std::strcmp(colTermENV, "24bit") == 0))) {
			colorMode = ColorMode::truecolor;
		} else if ((termENV != NULL) &&
		           ((std::strcmp(termENV, "iterm") == 0) || (std::strcmp(termENV, "linux-truecolor") == 0) ||
		            (std::strcmp(termENV, "gnome-truecolor") == 0) || (std::strcmp(termENV, "screen-truecolor") == 0) ||
		            (std::strcmp(termENV, "tmux-truecolor") == 0) || (std::strcmp(termENV, "xterm-truecolor") == 0) ||
		            (std::getenv("WT_SESSION") != NULL))) {
			colorMode = ColorMode::truecolor;
		} else if ((termENV != NULL) &&
		           (std::strcmp(termENV, "xterm-256color") || std::strcmp(termENV, "tmux-256color") ||
		            std::strcmp(termENV, "gnome-256color"))) {
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
				while (next.has_value()) {
					if (next.value() == "--lib") {
						if (isLib) {
							log->warn("The '--lib' flag has already been provided", None);
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
					} else if (next.value().starts_with("--vcs=")) {
						if (vcs.has_value()) {
							log->fatalError("The '--vcs' argument has already been provided", None);
						}
						if (next.value().length() > String::traits_type::length("--vcs=")) {
							vcs = next.value().substr(String::traits_type::length("--vcs="));
						} else {
							log->fatalError(
							    "Expected valid value after '--vcs='. This argument determines which version control system should be used for the new project. Use '--vcs=none' to disable this feature for this project",
							    None);
						}
					} else if (fs::exists(next.value())) {
						if (not fs::is_directory(next.value())) {
							log->fatalError("The provided path " + next.value() +
							                    " is not a directory and hence a project cannot be created in it",
							                next.value());
						}
						candPath = next;
					} else {
						log->fatalError("The provided path " + next.value() +
						                    " for creating the new project, does not exist",
						                next.value());
					}
					next = readNext();
				}
				cli::create_project(projectName, candPath.value_or(fs::current_path().string()), isLib,
				                    vcs.has_value() ? ((vcs.value() == "none") ? None : vcs) : "git");
			} else {
				log->fatalError("The 'new' command expects the name of the project to be created", None);
			}
			exitAfter = true;
		} else if (command == "setup") {
			auto next = readNext();
			if (not next.has_value()) {
				setup_path_in_env(true);
			} else {
				// TODO - Add support for setting up the toolchain
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
				cli::Error(
				    "Nothing to show here. Please provide name of the information to display. The available names are\n"
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
					cli::Error("Expected a valid target triple string after --target=", None);
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
					log->fatalError("Expected valid path after " + log->color("--sysroot="), None);
				}
			} else if (arg == "--sysroot") {
				if (hasNext()) {
					sysRoot = filter_quotes(getNext());
				} else {
					cli::Error("Expected a valid path for the sysroot after the --sysroot parameter", None);
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
					cli::Error(
					    "Expected argument after '--clang' which would be the path to the clang executable to be used",
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
					cli::Error("Expected argument after '--linker' which would be the path to the linker to be used",
					           None);
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
							cli::Error("Provided output path is not a directory! Please provide path to a directory",
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
						outputPath = out;
					}
				} else {
					cli::Error(
					    "Output path is not provided! Please provide path to a directory "
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
			} else if (arg == "--freestanding") {
				stdLibPath     = None;
				isFreestanding = true;
			} else if (arg == "--save-docs") {
				saveDocs = true;
			} else if (arg.starts_with("--panic=")) {
				auto panicVal = arg.substr(String::traits_type::length("--panic="));
				if (panicVal == "resume") {
					panicStrategy = PanicStrategy::resume;
				} else if (panicVal == "exit-thread") {
					panicStrategy = PanicStrategy::exitThread;
				} else if (panicVal == "exit-program") {
					panicStrategy = PanicStrategy::exitProgram;
				} else if (panicVal == "handler") {
					panicStrategy = PanicStrategy::handler;
				} else {
					log->fatalError("Invalid panic strategy found: " + log->color(arg) + ". The possible values are " +
					                    log->color("resume") + ", " + log->color("exit-thread") + ", " +
					                    log->color("exit-program") + " and " + log->color("handler"),
					                None);
				}
			} else if (arg.starts_with("--mode=")) {
				auto modeVal = filter_quotes(arg.substr(String::traits_type::length("--mode=")));
				if (modeVal == "debug") {
					buildMode = BuildMode::debug;
				} else if (modeVal == "release") {
					buildMode = BuildMode::release;
				} else if (modeVal == "releaseWithDebugInfo") {
					buildMode = BuildMode::releaseWithDebugInfo;
				} else {
					log->fatalError("Invalid value for the argument " + log->color("--mode") +
					                    ". The possible values are " + log->color("debug") + ", " +
					                    log->color("release") + " and " + log->color("releaseWithDebugInfo"),
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
						log->fatalError("Unrecognised argument " + log->color(arg) +
						                    " and it is also not an existing file or directory",
						                arg);
					}
					log->fatalError("Provided path " + log->color(arg) +
					                    " does not exist! Please provide path to a file or directory",
					                arg);
				}
			}
		}
		if (exitAfter && (proceed < count)) {
			String otherArgs;
			for (usize i = proceed; i < count; i++) {
				otherArgs += log->color(args[proceed]);
				if (i != (count - 1)) {
					otherArgs += ", ";
				}
			}
			log->warn("Ignoring additional arguments provided: " + otherArgs, None);
		}
		if (is_freestanding() && (not has_panic_strategy())) {
			log->fatalError(
			    "The " + log->color("--freestanding") +
			        " flag was provided, but panic strategy has not been provided. You can provide the panic strategy using one of the following arguments: " +
			        log->color("--panic=resume") + '\n' + log->color("--panic=exit-thread") + '\n' +
			        log->color("--panic=exit-program") + '\n' + log->color("--panic=handler") + "\nRun " +
			        log->color("qat help --panic") + " to see what each of these options mean",
			    None);
		}
		if ((not outputPath.has_value()) && (not exitAfter)) {
			if ((is_workflow_build() || is_workflow_bundle() || is_workflow_analyse() || is_workflow_run()) &&
			    (paths.size() == 1)) {
				if ((fs::is_regular_file(paths[0]) && (paths[0].extension() == ".qat")) || fs::is_directory(paths[0])) {
					auto candParent =
					    fs::is_directory(paths[0]) ? fs::path(paths[0]) : fs::path(paths[0]).parent_path();
					if (fs::exists(candParent)) {
						auto outDir = candParent / ".qatcache";
						outputPath  = outDir;
					} else {
						log->fatalError("The path " + log->color(candParent.string()) + " does not exist", candParent);
					}
				}
			}
			if (not outputPath.has_value()) {
				outputPath = fs::current_path() / ".qatcache";
			}
			if (not fs::exists(outputPath.value())) {
				std::error_code err;
				fs::create_directories(outputPath.value(), err);
				if (err) {
					log->fatalError("Could not create the directory " + outputPath.value().string() +
					                    " for output. The error is: " + err.message(),
					                outputPath.value());
				}
			}
		}
	}
}

} // namespace qat::cli
