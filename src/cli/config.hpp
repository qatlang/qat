#ifndef QAT_CLI_CONFIG_HPP
#define QAT_CLI_CONFIG_HPP

#include <helpers/files.hpp>
#include <helpers/integers.hpp>
#include <helpers/maybe.hpp>
#include <helpers/string.hpp>
#include <helpers/vec.hpp>
#include <llvm/Support/VersionTuple.h>

namespace qat::cli {

enum class ColorMode { none, truecolor, color256 };

enum class BuildMode { debug, release, releaseWithDebugInfo };

enum class PanicStrategy { resume, exitThread, exitProgram, handler, none };

class Config {
  private:
	static Config* instance;

	FilePath qatDirPath;
	String   buildCommit;
	String   invokePath;

	Maybe<FilePath> coreLibPath;
	Maybe<FilePath> toolchainPath;

	Maybe<FilePath> outputPath;

	Vec<FilePath> paths;
	Maybe<String> targetTriple;
	Maybe<String> clangPath;
	Maybe<String> linkerPath;
	Maybe<String> sysRoot;
	Maybe<String> cpuName;
	Maybe<String> cpuFeatures;

	llvm::VersionTuple versionTuple;

	bool exitAfter       = false;
	bool verbose         = false;
	bool saveDocs        = false;
	bool showReport      = false;
	bool buildWorkflow   = false;
	bool runWorkflow     = false;
	bool bundleWorkflow  = false;
	bool analyseWorkflow = false;
	bool clearLLVMFiles  = false;
	bool isFreestanding  = false;
	bool isNoCoreLib     = false;
	bool diagnostic      = false;

	ColorMode colorMode = ColorMode::color256;
	BuildMode buildMode = BuildMode::debug;

	PanicStrategy panicStrategy = PanicStrategy::none;

	Maybe<bool> buildShared;
	Maybe<bool> buildStatic;

  public:
	Config(u64 count, const char** args);

	static Config* initialise(u64 count, const char** args);

	inline static Config const* get() { return Config::instance; }

	inline static bool has_instance() { return Config::instance != nullptr; }

	static String filter_quotes(String value);

	void find_corelib_and_toolchain();

	void setup_path_in_env(bool isSetupCmd);

	/** Behaviour specific functions */

	bool is_workflow_build() const { return buildWorkflow; }

	bool is_workflow_run() const { return runWorkflow; }

	bool is_workflow_analyse() const { return analyseWorkflow; }

	bool is_workflow_bundle() const { return bundleWorkflow; }

	bool should_show_report() const { return showReport; }

	bool is_verbose() const { return verbose; }

	bool should_save_docs() const { return saveDocs; }

	bool has_output_path() const { return outputPath.has_value(); }

	bool clear_llvm() const { return clearLLVMFiles; }

	bool has_target_triple() const { return targetTriple.has_value(); }

	bool has_cpu_name() const { return cpuName.has_value(); }

	bool has_cpu_features() const { return cpuFeatures.has_value(); }

	bool has_std_lib_path() const { return coreLibPath.has_value(); }

	bool has_toolchain_path() const { return toolchainPath.has_value(); }

	bool is_freestanding() const { return isFreestanding; }

	bool is_no_corelib_enabled() const { return isNoCoreLib || isFreestanding; }

	ColorMode color_mode() const { return colorMode; }

	bool is_no_color_mode() const { return colorMode == ColorMode::none; }

	bool is_build_mode_debug() const { return buildMode == BuildMode::debug; }

	bool is_build_mode_release() const { return buildMode == BuildMode::release; }

	bool should_have_debug_info() const {
		return (buildMode == BuildMode::releaseWithDebugInfo) || (buildMode == BuildMode::debug);
	}

	bool has_panic_strategy() const { return panicStrategy != PanicStrategy::none; }

	PanicStrategy get_panic_strategy() const { return panicStrategy; }

	bool has_sysroot() const { return sysRoot.has_value(); }

	bool has_clang_path() const { return clangPath.has_value(); }

	bool has_linker_path() const { return linkerPath.has_value(); }

	bool should_build_static() const { return buildShared.has_value() ? buildStatic.has_value() : true; }

	bool should_build_shared() const { return buildShared.value_or(false); }

	bool should_exit() const { return exitAfter; }

	bool should_do_diagnostics() const { return diagnostic; }

	String get_target_triple() const { return targetTriple.value_or(LLVM_HOST_TRIPLE); }

	String get_cpu_name() const { return cpuName.value(); }

	String get_cpu_features() const { return cpuFeatures.value(); }

	String get_sysroot() const { return sysRoot.value(); }

	String get_clang_path() const { return clangPath.value(); }

	String get_linker_path() const { return linkerPath.value(); }

	FilePath get_std_lib_path() const { return coreLibPath.value(); }

	FilePath get_toolchain_path() const { return toolchainPath.value(); }

	FilePath get_output_path() const { return outputPath.value_or(fs::current_path()); }

	Vec<FilePath> get_paths() const { return paths; }

	const llvm::VersionTuple& get_version_tuple() const { return versionTuple; }

	~Config() = default;
};

} // namespace qat::cli

#endif
