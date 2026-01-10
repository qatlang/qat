#include "./sitter.hpp"
#include "./IR/qat_module.hpp"
#include "./IR/stdlib.hpp"
#include "./IR/type_id.hpp"
#include "./IR/value.hpp"
#include "./ast/types/qat_type.hpp"
#include "./cli/config.hpp"
#include "./cli/logger.hpp"
#include "./lexer/lexer.hpp"
#include "./lexer/token_type.hpp"
#include "./parser/parser.hpp"
#include "./show.hpp"
#include "./utils/identifier.hpp"
#include "./utils/profiler.hpp"
#include "./utils/run_command.hpp"
#include "./utils/visibility.hpp"

#include <chrono>
#include <llvm/ADT/StringMap.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>

#if OS_IS_WINDOWS
#include "../utils/macros.hpp"
#if RUNTIME_IS_MINGW
#include <sdkddkver.h>
#include <windows.h>
#elif RUNTIME_IS_MSVC
#include <SDKDDKVer.h>
#include <Windows.h>
#endif
#endif

#define ONE_SECOND_IN_NANO 1000000000
#define ONE_MILLI_IN_NANO  1000000
#define ONE_MICRO_IN_NANO  1000

namespace qat {

QatSitter* QatSitter::instance = nullptr;

QatSitter::QatSitter()
    : ctx(ir::Ctx::create()), Lexer(lexer::Lexer::get(ctx)), Parser(parser::Parser::get(ctx, Lexer->get_tokens())) {
	ctx->sitter = this;
}

QatSitter* QatSitter::get() {
	if (instance) {
		return instance;
	}
	instance = new QatSitter();
	return instance;
}

void QatSitter::display_stats() {
	auto&             log = Logger::get();
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << static_cast<u64>(static_cast<double>(lexer::Lexer::lineCount) * ONE_SECOND_IN_NANO /
	                       lexer::Lexer::timeInNanoseconds);
	log->diagnostic("Lexer speed   -> " + ss.str() + " lines/s");
	ss.str("");
	ss.clear();
	ss << static_cast<u64>(static_cast<double>(lexer::Lexer::lineCount) * ONE_SECOND_IN_NANO /
	                       parser::Parser::timeInNanoseconds);
	auto parserMsg = "Parser speed  -> " + ss.str() + " lines/s & ";
	ss.str("");
	ss.clear();
	ss << static_cast<u64>(static_cast<double>(parser::Parser::tokenCount) * ONE_SECOND_IN_NANO /
	                       parser::Parser::timeInNanoseconds);
	parserMsg += ss.str() + " tokens/s";
	log->diagnostic(parserMsg);
	if (ctx->clangAndLinkTimeInNanoseconds.has_value()) {
		ss.str("");
		ss.clear();
		ss << static_cast<u64>(static_cast<double>(lexer::Lexer::lineCount) * ONE_SECOND_IN_NANO /
		                       ctx->qatCompileTimeInNanoseconds.value());
		log->diagnostic("Compile speed -> " + ss.str() + " lines/s");
	}
	auto timeToString = [](u64 timeInNanos) {
		if (timeInNanos > ONE_SECOND_IN_NANO) {
			return std::to_string(static_cast<double>(timeInNanos) / ONE_SECOND_IN_NANO) + " seconds";
		} else if (timeInNanos > ONE_MILLI_IN_NANO) {
			return std::to_string(static_cast<double>(timeInNanos) / ONE_MILLI_IN_NANO) + " milliseconds";
		} else if (timeInNanos > ONE_MICRO_IN_NANO) {
			return std::to_string(static_cast<double>(timeInNanos) / ONE_MICRO_IN_NANO) + " microseconds";
		} else {
			return std::to_string(timeInNanos) + " nanoseconds";
		}
	};
	log->diagnostic("Lexer time    -> " + timeToString(lexer::Lexer::timeInNanoseconds));
	log->diagnostic("Parser time   -> " + timeToString(parser::Parser::timeInNanoseconds));
	if (ctx->qatCompileTimeInNanoseconds.has_value() && ctx->clangAndLinkTimeInNanoseconds.has_value()) {
		log->diagnostic("Compile time  -> " + timeToString(ctx->qatCompileTimeInNanoseconds.value()));
		log->diagnostic("clang & lld   -> " + timeToString(ctx->clangAndLinkTimeInNanoseconds.value()));
	}
}

void QatSitter::initialise() {
	auto* config = cli::Config::get();
	auto& log    = Logger::get();
	SHOW("Module count: " << ir::Mod::allModules.size())
	for (const auto& path : config->get_paths()) {
		SHOW("Handling path for " << path)
		handle_path(path, ctx);
	}
	SHOW("Module count: " << ir::Mod::allModules.size())
	if (ir::Mod::allModules.size() > 0) {
		SHOW("Module " << ir::Mod::allModules[0])
		SHOW("Module name " << ir::Mod::allModules[0]->name.value)
	}
	if (config->has_std_lib_path() && ctx->stdLibPossiblyRequired) {
		handle_path(config->get_std_lib_path(), ctx);
		if (ir::Mod::has_file_module(config->get_std_lib_path())) {
			ir::StdLib::stdLib = ir::Mod::get_file_module(config->get_std_lib_path());
		}
	}
	auto cfg = cli::Config::get();
	SHOW("Module count: " << ir::Mod::allModules.size())
	if (config->is_workflow_build() || config->is_workflow_analyse()) {
		llvm::InitializeAllTargetInfos();
		llvm::InitializeAllTargets();
		llvm::InitializeAllTargetMCs();
		llvm::InitializeAllAsmParsers();
		llvm::InitializeAllAsmPrinters();
		String errStr;
		auto   target = llvm::TargetRegistry::lookupTarget(cfg->get_target_triple(), errStr);
		if (not target) {
			ctx->Error("The target triple " + cfg->get_target_triple() +
			               " is invalid. The error returned from LLVM is " + errStr,
			           None);
		}
		llvm::TargetOptions targetOptions;
		String              cpuName = "generic";
		String              cpuFeatures;
		if (not cfg->has_target_triple() && not cfg->has_cpu_name()) {
			cpuName = llvm::sys::getHostCPUName().str();
			SHOW("CPU name is " << cpuName);
			auto features = llvm::sys::getHostCPUFeatures();
			for (auto& item : features) {
				if (item.getValue()) {
					SHOW("Enabling CPU feature " << item.getKey().str());
					if (not cpuFeatures.empty()) {
						cpuFeatures += ",";
					}
					cpuFeatures += ("+" + item.getKey()).str();
				}
			}
		}
		if (cfg->has_cpu_name()) {
			cpuName = cfg->get_cpu_name();
		}
		if (cfg->has_cpu_features()) {
			cpuFeatures = cfg->get_cpu_features();
		}
		ctx->targetMachine = target->createTargetMachine(cfg->get_target_triple(), cpuName, cpuFeatures, targetOptions,
		                                                 llvm::Reloc::PIC_);
		auto qatStartTime  = std::chrono::high_resolution_clock::now();
		SHOW("Module count: " << ir::Mod::allModules.size())
		for (auto* entity : fileEntities) {
			entity->node_handle_fs_brings(ctx);
		}
		SHOW("Module count: " << ir::Mod::allModules.size())
		for (auto* entity : fileEntities) {
			entity->node_create_modules(ctx);
		}
		SHOW("Module count: " << ir::Mod::allModules.size())
		for (auto* entity : fileEntities) {
			SHOW("Create Entity: " << entity->get_name())
			entity->node_create_entities(ctx);
		}
		SHOW("Module count: " << ir::Mod::allModules.size())
		for (auto* entity : fileEntities) {
			SHOW("Update Entity Dependencies: " << entity->get_name())
			entity->node_update_dependencies(ctx);
		}
		SHOW("Module count: " << ir::Mod::allModules.size())
		bool atleastOneEntityDone  = true;
		bool hasIncompleteEntities = true;
		while (hasIncompleteEntities && atleastOneEntityDone) {
			atleastOneEntityDone  = false;
			hasIncompleteEntities = false;
			for (usize i = 0; i < ir::Mod::allModules.size(); i++) {
				auto itMod = ir::Mod::allModules[i];
				SHOW("Module count: " << ir::Mod::allModules.size())
				SHOW("Module " << itMod << ", bool := " << (bool)itMod)
				SHOW("Module name " << itMod->name.value)
				SHOW("Module referrable name: " << itMod->get_referrable_name())
				for (auto ent : itMod->entityEntries) {
					SHOW("Entity name: " << (ent->name ? ent->name.value().value : ""))
					if (not ent->are_all_phases_complete()) {
						if (ent->is_ready_for_next_phase()) {
							ent->do_next_phase(itMod, ctx);
							SHOW("do_next_phase complete")
							atleastOneEntityDone = true;
						}
						SHOW("Checking are_all_phases_complete")
						if (not ent->are_all_phases_complete()) {
							hasIncompleteEntities = true;
						}
						SHOW("Done are_all_phases_complete")
					}
					SHOW("Incrementing iteration count")
					ent->iterations++;
				}
			}
		}
		if ((not atleastOneEntityDone) && hasIncompleteEntities) {
			Vec<ir::QatError> errors;
			for (auto* iterMod : ir::Mod::allModules) {
				for (auto ent : iterMod->entityEntries) {
					if (not ent->are_all_phases_complete()) {
						String                     depStr;
						usize                      incompleteDepCount = 0;
						std::set<ir::EntityState*> ents;
						for (auto dep : ent->dependencies) {
							if (ents.contains(dep.entity)) {
								continue;
							}
							ents.insert(dep.entity);
							if (dep.entity->supportsChildren ? (dep.entity->status != ir::EntityStatus::childrenPartial)
							                                 : (dep.entity->status != ir::EntityStatus::complete)) {
								if (dep.entity->name.has_value()) {
									depStr +=
									    (dep.type == ir::DependType::partial ? "- depends partially on "
									                                         : "- depends on ") +
									    ctx->color(iterMod->get_fullname_with_child(dep.entity->name.value().value)) +
									    +" at " + dep.entity->name.value().range->start_to_string() + "\n";
								} else {
									depStr += String(dep.type == ir::DependType::partial ? "- Depends partially on "
									                                                     : " - Depends on ") +
									          "unnamed " + ir::entity_type_to_string(ent->type) +
									          (dep.entity->astNode
									               ? (" at " + dep.entity->astNode->fileRange->start_to_string())
									               : "") +
									          "\n";
								}
								incompleteDepCount++;
							}
						}
						errors.push_back(ir::QatError(
						    "This " + String(ent->status == ir::EntityStatus::partial ? "partially created " : "") +
						        ir::entity_type_to_string(ent->type) + " " +
						        (ent->name.has_value() ? ctx->color(ent->name.value().value) + " " : "") +
						        "could not be finalised as its dependencies were not resolved properly. This entity has " +
						        ctx->color(std::to_string(incompleteDepCount)) + " incomplete dependenc" +
						        (incompleteDepCount > 1 ? "ies" : "y") +
						        ((incompleteDepCount > 0)
						             ? ((incompleteDepCount > 1 ? ". The dependencies are\n" : "\n") + depStr)
						             : ""),
						    ent->astNode
						        ? ent->astNode->fileRange
						        : (ent->name.has_value() ? Maybe<FileRangePtr>(ent->name.value().range) : None)));
					}
				}
			}
			ctx->Errors(errors);
		}
		ir::TypeInfo::finalise_type_infos(ctx);
		for (auto* entity : fileEntities) {
			entity->setup_llvm_file(ctx);
		}
		auto qatCompileTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
		                          std::chrono::high_resolution_clock::now() - qatStartTime)
		                          .count();
		ctx->qatCompileTimeInNanoseconds = qatCompileTime;
		auto clear_llvm_files            = [&] {
            if (cfg->clear_llvm()) {
                for (const auto& llPath : ctx->llvmOutputPaths) {
                    fs::remove(llPath);
                }
                if (cfg->has_output_path() && fs::exists(cfg->get_output_path() / "llvm")) {
                    fs::remove_all(cfg->get_output_path() / "llvm");
                }
                log->say("Cleared LLVM files");
            }
		};
		if (cfg->is_workflow_build()) {
			llvm::PassBuilder passBuilder;
			const auto        clangStartTime = std::chrono::high_resolution_clock::now();
			for (auto* entity : fileEntities) {
				entity->compile_to_object(ctx, passBuilder);
			}
			ir::Mod::find_native_library_paths();
			for (auto* entity : fileEntities) {
				entity->handle_native_libs(ctx);
			}
			for (auto* entity : fileEntities) {
				entity->bundle_modules(ctx);
			}
			ctx->clangAndLinkTimeInNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
			                                         std::chrono::high_resolution_clock::now() - clangStartTime)
			                                         .count();
			display_stats();
			SHOW("Displayed stats")
			ctx->write_json_result(true);
			clear_llvm_files();
			SHOW("Cleared llvm files")
			if (cfg->is_workflow_run() && not ctx->executablePaths.empty()) {
				if (llvm::Triple(cfg->get_target_triple()) != llvm::Triple(LLVM_HOST_TRIPLE)) {
					ctx->Error("The target provided for compilation is " + ctx->color(cfg->get_target_triple()) +
					               " which does not match the host target triplet of this compiler, which is " +
					               ctx->color(LLVM_HOST_TRIPLE) + ". Cannot run built executables due to this mismatch",
					           None);
				}
				for (const auto& exePath : ctx->executablePaths) {
					std::cout << "\n===== Output of \"" << exePath.lexically_relative(fs::current_path()).string()
					          << "\"\n";
					auto exitCode = run_command_with_output(fs::absolute(exePath).string(), {});
					std::cout << "\n===== Status Code: " << std::to_string(exitCode) << "\n";
					if (exitCode) {
						std::cout << "\nThe built executable at " + ctx->color(exePath.string()) + " exited with error";
					}
				}
			}
			SHOW("Workflow run check complete")
		} else {
			display_stats();
			clear_llvm_files();
			ctx->write_json_result(true);
		}
	}
	SHOW("Writing profiler data")
	Profiler::write_to_file((cfg->get_output_path() / "profiler.txt").string());
	SHOW("Profiler data written")
	SHOW("Sitter reached completion")
}

void QatSitter::remove_entity_with_path(FilePath const& path) {
	for (auto item = fileEntities.begin(); item != fileEntities.end(); item++) {
		if (((*item)->get_mod_type() == ir::ModuleType::file || (*item)->get_mod_type() == ir::ModuleType::folder) &&
		    fs::equivalent(FilePath((*item)->get_file_path()), path)) {
			fileEntities.erase(item);
			return;
		}
	}
}

Maybe<Pair<String, FilePath>> QatSitter::detect_lib_file(FilePath const& path) {
	if (fs::is_directory(path)) {
		for (const auto& item : fs::directory_iterator(path)) {
			if (fs::is_regular_file(item)) {
				auto name = item.path().filename().string();
				if (name.ends_with(".lib.qat")) {
					return Pair<String, FilePath>(name.substr(0, name.length() - 8),
					                              item); // NOLINT(readability-magic-numbers)
				}
			}
		}
	} else if (fs::is_regular_file(path)) {
		auto name = path.filename().string();
		if (name.ends_with(".lib.qat")) {
			SHOW("lib file detected: " << name.substr(0, name.length() - 8))
			return Pair<String, FilePath>(name.substr(0, name.length() - 8), path); // NOLINT(readability-magic-numbers)
		}
	}
	return None;
}

bool QatSitter::is_name_valid(String const& name) {
	auto lexRes = lexer::Lexer::word_to_token(name, nullptr);
	return (lexRes.has_value() && lexRes.value().type == lexer::TokenType::identifier);
}

void QatSitter::handle_path(FilePath const& mainPath, ir::Ctx* irCtx) {
	Vec<FilePath>                                  broughtPaths;
	Vec<FilePath>                                  memberPaths;
	auto*                                          cfg                    = cli::Config::get();
	std::function<void(ir::Mod*, FilePath const&)> recursiveModuleCreator = [&](ir::Mod*        parentMod,
	                                                                            FilePath const& path) {
		for (auto const& item : fs::directory_iterator(path)) {
			if (fs::is_directory(item) && not fs::equivalent(item, cfg->get_output_path()) &&
			    not ir::Mod::has_folder_module(item)) {
				auto libCheckRes = detect_lib_file(item);
				if (libCheckRes.has_value()) {
					if (not is_name_valid(libCheckRes->first)) {
						irCtx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
						                 irCtx->color(libCheckRes->first) + " which is illegal",
						             None);
					}
					Lexer->change_file(fs::absolute(libCheckRes->second));
					Lexer->analyse();
					auto parseRes(Parser->begin_parsing());
					for (const auto& bPath : Parser->get_imported_paths()) {
						broughtPaths.push_back(bPath);
					}
					for (const auto& mPath : Parser->get_member_paths()) {
						memberPaths.push_back(mPath);
					}
					Parser->clear_imported_paths();
					Parser->clear_member_paths();
					fileEntities.push_back(ir::Mod::create_root_lib(
					    parentMod, fs::absolute(libCheckRes->second), path,
					    Identifier(libCheckRes->first,
					               FileRange::from_path(std::construct_at(OwnNormal(String), libCheckRes->second))),
					    std::move(parseRes), VisibilityInfo::pub(), irCtx));
				} else {
					auto dirQatChecker = [](fs::directory_entry const& entry) {
						bool foundQatFile = false;
						for (auto const& dirItem : fs::directory_iterator(entry)) {
							if (dirItem.is_regular_file() && dirItem.path().extension() == ".qat") {
								foundQatFile = true;
								break;
							}
						}
						return foundQatFile;
					};
					if (dirQatChecker(item)) {
						auto* subfolder = ir::Mod::create_submodule(
						    parentMod, item.path(), path,
						    Identifier(fs::absolute(item.path().filename()).string(),
						               FileRange::from_path(std::construct_at(OwnNormal(String), item.path()))),
						    ir::ModuleType::folder, VisibilityInfo::pub(), irCtx);
						fileEntities.push_back(subfolder);
						recursiveModuleCreator(subfolder, item);
					} else {
						recursiveModuleCreator(parentMod, item);
					}
				}
			} else if (fs::is_regular_file(item) && not ir::Mod::has_file_module(item) &&
			           (item.path().extension() == ".qat")) {
				auto libCheckRes = detect_lib_file(item);
				if (libCheckRes.has_value() && not is_name_valid(libCheckRes.value().first)) {
					irCtx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
					                 irCtx->color(libCheckRes->first) + " which is illegal",
					             None);
				}
				Lexer->change_file(item.path().string());
				Lexer->analyse();
				auto parseRes(Parser->begin_parsing());
				for (const auto& bPath : Parser->get_imported_paths()) {
					broughtPaths.push_back(bPath);
				}
				for (const auto& mPath : Parser->get_member_paths()) {
					memberPaths.push_back(mPath);
				}
				Parser->clear_imported_paths();
				Parser->clear_member_paths();
				if (libCheckRes.has_value()) {
					fileEntities.push_back(ir::Mod::create_root_lib(
					    parentMod, fs::absolute(item), path,
					    Identifier(libCheckRes->first,
					               FileRange::from_path(std::construct_at(OwnNormal(String), libCheckRes->second))),
					    std::move(parseRes), VisibilityInfo::pub(), irCtx));
				} else {
					fileEntities.push_back(ir::Mod::create_file_mod(
					    parentMod, fs::absolute(item), path,
					    Identifier(item.path().filename().string(),
					               FileRange::from_path(std::construct_at(OwnNormal(String), item.path()))),
					    std::move(parseRes), VisibilityInfo::pub(), irCtx));
				}
			}
		}
	};
	// FIXME - Check if modules are already part of another module
	if (fs::is_directory(mainPath) && not fs::equivalent(mainPath, cfg->get_output_path()) &&
	    not ir::Mod::has_folder_module(mainPath)) {
		auto libCheckRes = detect_lib_file(mainPath);
		if (libCheckRes.has_value()) {
			if (not is_name_valid(libCheckRes.value().first)) {
				irCtx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
				                 irCtx->color(libCheckRes->first) + " which is illegal",
				             None);
			}
			Lexer->change_file(libCheckRes->second);
			Lexer->analyse();
			auto parseRes(Parser->begin_parsing());
			for (const auto& bPath : Parser->get_imported_paths()) {
				broughtPaths.push_back(bPath);
			}
			for (const auto& mPath : Parser->get_member_paths()) {
				memberPaths.push_back(mPath);
			}
			Parser->clear_imported_paths();
			Parser->clear_member_paths();
			fileEntities.push_back(ir::Mod::create_file_mod(
			    nullptr, libCheckRes->second, mainPath,
			    Identifier(libCheckRes->first,
			               FileRange::from_path(std::construct_at(OwnNormal(String), libCheckRes->second))),
			    std::move(parseRes), VisibilityInfo::pub(), irCtx));
		} else {
			auto* subfolder =
			    ir::Mod::create(Identifier(mainPath.filename().string(),
			                               FileRange::from_path(std::construct_at(OwnNormal(String), mainPath))),
			                    mainPath, mainPath.parent_path(), ir::ModuleType::folder, VisibilityInfo::pub(), irCtx);
			fileEntities.push_back(subfolder);
			recursiveModuleCreator(subfolder, mainPath);
		}
	} else if (fs::is_regular_file(mainPath) && not ir::Mod::has_file_module(mainPath)) {
		SHOW("Found regular file")
		auto libCheckRes = detect_lib_file(mainPath);
		if (libCheckRes.has_value() && not is_name_valid(libCheckRes.value().first)) {
			irCtx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
			                 irCtx->color(libCheckRes->first) + " which is illegal",
			             None);
		}
		Lexer->change_file(mainPath);
		Lexer->analyse();
		auto parseRes(Parser->begin_parsing());
		for (const auto& bPath : Parser->get_imported_paths()) {
			broughtPaths.push_back(bPath);
		}
		for (const auto& mPath : Parser->get_member_paths()) {
			memberPaths.push_back(mPath);
		}
		Parser->clear_imported_paths();
		Parser->clear_member_paths();
		if (libCheckRes.has_value()) {
			fileEntities.push_back(ir::Mod::create_root_lib(
			    nullptr, fs::absolute(mainPath), mainPath.parent_path(),
			    Identifier(libCheckRes->first,
			               FileRange::from_path(std::construct_at(OwnNormal(String), libCheckRes->second))),
			    std::move(parseRes), VisibilityInfo::pub(), irCtx));
		} else {
			fileEntities.push_back(ir::Mod::create_file_mod(
			    nullptr, fs::absolute(mainPath), mainPath.parent_path(),
			    Identifier(mainPath.filename().string(),
			               FileRange::from_path(std::construct_at(OwnNormal(String), mainPath))),
			    std::move(parseRes), VisibilityInfo::pub(), irCtx));
		}
	}
	for (const auto& bPath : broughtPaths) {
		handle_path(bPath, irCtx);
	}
	broughtPaths.clear();
	for (const auto& mPath : memberPaths) {
		remove_entity_with_path(mPath);
	}
	memberPaths.clear();
}

void QatSitter::destroy() {
	delete Lexer;
	delete Parser;
	SHOW("Deleted lexer and parser")
	ir::Mod::clear_all();
	SHOW("ir::Mod complete")
	ast::Node::clear_all();
	SHOW("ast::Node complete")
	ast::Type::clear_all();
	SHOW("ast::Type complete")
	ir::Value::clear_all();
	SHOW("ir::Value complete")
	ir::Type::clear_all();
	SHOW("ir::Type complete")
}

QatSitter::~QatSitter() { destroy(); }

} // namespace qat
