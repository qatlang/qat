#ifndef QAT_IR_CONTEXT_HPP
#define QAT_IR_CONTEXT_HPP

#include "../cli/color.hpp"
#include "../cli/config.hpp"
#include "../utils/file_range.hpp"
#include "./qat_module.hpp"
#include "function.hpp"
#include "clang/Basic/TargetInfo.h"

#include <chrono>
#include <llvm/IR/ConstantFolder.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <string>

using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

namespace qat {

class QatSitter;

namespace ast {
struct VisibilitySpec;
}

} // namespace qat

namespace qat::ir {

enum class GenericEntityType {
	function,
	memberFunction,
	coreType,
	mixType,
	typeDefinition,
};

struct GenericEntityMarker {
	String					  name;
	GenericEntityType		  type;
	FileRange				  fileRange;
	u64						  warningCount = 0;
	Vec<ir::GenericArgument*> generics{};

	bool hasGenericParameter(const String& name) const {
		for (auto* gen : generics) {
			if (gen->is_same(name)) {
				return true;
			}
		}
		return false;
	}

	ir::GenericArgument* getGenericParameter(const String& name) const {
		for (auto* gen : generics) {
			if (gen->is_same(name)) {
				return gen;
			}
		}
		return nullptr;
	}
};

class CodeProblem {
	bool			 isError;
	String			 message;
	Maybe<FileRange> range;

  public:
	CodeProblem(bool isError, String message, Maybe<FileRange> range);
	operator Json() const;
};

class QatError {
	friend class ir::Ctx;
	String			 message;
	Maybe<FileRange> fileRange;

  public:
	QatError();
	QatError(String message, Maybe<FileRange> fileRange);

	QatError& add(String value);
	QatError& colored(String value);
	void	  setRange(FileRange range);
};

class Ctx {
	friend class qat::QatSitter;

  private:
	using IRBuilderTy = llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>;

	Vec<ir::Mod*> modulesWithErrors;

	useit bool module_has_errors(ir::Mod* cand) {
		for (auto* module : modulesWithErrors) {
			if (module->get_id() == cand->get_id()) {
				return true;
			}
		}
		return false;
	}

	Pair<usize, Vec<std::tuple<String, u64, u64>>> get_range_content(FileRange const& _range) const;

	void print_range_content(FileRange const& fileRange, bool isError, bool isContentError) const;

	QatSitter* sitter = nullptr;

	// NOTE - Single instance for now
	static Ctx* instance;

  public:
	Ctx();

	static Ctx* New() {
		if (instance) {
			return instance;
		}
		instance = new Ctx();
		return instance;
	}

	llvm::LLVMContext		llctx;
	clang::TargetInfo*		clangTargetInfo;
	Maybe<llvm::DataLayout> dataLayout;
	IRBuilderTy				builder;
	Vec<fs::path>			executablePaths;

	// META
	bool							 hasMain;
	bool							 stdLibRequired = false;
	mutable u64						 stringCount;
	Vec<fs::path>					 llvmOutputPaths;
	Vec<String>						 nativeLibsToLink;
	mutable Vec<GenericEntityMarker> allActiveGenerics;
	mutable Vec<usize>				 lastMainActiveGeneric;
	mutable Vec<CodeProblem>		 codeProblems;
	mutable Vec<usize>				 binarySizes;
	mutable Maybe<u64>				 qatCompileTimeInMs;
	mutable Maybe<u64>				 clangAndLinkTimeInMs;

	useit bool				   has_active_generic() const { return !allActiveGenerics.empty(); }
	useit GenericEntityMarker& get_active_generic() const { return allActiveGenerics.back(); }
	useit bool				   has_generic_parameter_in_entity(String const& name) const;
	useit GenericArgument*	   get_generic_parameter_from_entity(String const& name) const;

	void add_active_generic(GenericEntityMarker marker, bool main) {
		if (main) {
			lastMainActiveGeneric.push_back(allActiveGenerics.size());
		}
		allActiveGenerics.push_back(marker);
	}

	void remove_active_generic() {
		if ((!lastMainActiveGeneric.empty()) && (allActiveGenerics.size() - 1 == lastMainActiveGeneric.back())) {
			lastMainActiveGeneric.pop_back();
		}
		allActiveGenerics.pop_back();
	}

	useit String joinActiveGenericNames(bool highlight) const {
		String result;
		for (usize i = 0; i < allActiveGenerics.size(); i++) {
			result.append(highlight ? color(allActiveGenerics.at(i).name) : allActiveGenerics.at(i).name);
			if (i < allActiveGenerics.size() - 1) {
				result.append(" and ");
			}
		}
		return result;
	}

	useit clang::LangAS get_language_address_space() const {
		if (dataLayout) {
			return clang::getLangASFromTargetAS(dataLayout->getProgramAddressSpace());
		} else {
			return clang::LangAS::Default;
		}
	}

	useit String get_global_string_name() const {
		auto res = "qat'str'" + std::to_string(stringCount);
		stringCount++;
		return res;
	}
	useit llvm::GlobalValue::LinkageTypes getGlobalLinkageForVisibility(VisibilityInfo const& visibInfo) const;

	void add_exe_path(fs::path path);
	void add_binary_size(usize size);
	void write_json_result(bool status) const;

	void finalise_errors();
	void add_error(ir::Mod* activeMod, const String& message, Maybe<FileRange> fileRange,
				   Maybe<Pair<String, FileRange>> pointTo = None);
	void Error(ir::Mod* activeMod, const String& message, Maybe<FileRange> fileRange,
			   Maybe<Pair<String, FileRange>> pointTo = None);
	void Error(const String& message, Maybe<FileRange> fileRange, Maybe<Pair<String, FileRange>> pointTo = None);
	void Errors(ir::Mod* activeMod, Vec<QatError> errors);
	void Errors(Vec<QatError> errors);
	void Warning(const String& message, const FileRange& fileRange);

	static String color(String const& message) {
		auto* cfg = cli::Config::get();
		return (cfg->is_no_color_mode() ? "`" : String(colors::bold) + cli::get_color(cli::Color::yellow)) + message +
			   (cfg->is_no_color_mode() ? "`" : String(colors::reset) + cli::get_color(cli::Color::white));
	}
	static String highlightWarning(const String& message);
	~Ctx() = default;
};

} // namespace qat::ir

#endif
