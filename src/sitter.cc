#include "./sitter.hpp"
#include "./IR/stdlib.hpp"
#include "./show.hpp"
#include "IR/qat_module.hpp"
#include "IR/value.hpp"
#include "ast/types/qat_type.hpp"
#include "cli/config.hpp"
#include "cli/logger.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token_type.hpp"
#include "parser/parser.hpp"
#include "utils/find_executable.hpp"
#include "utils/identifier.hpp"
#include "utils/run_command.hpp"
#include "utils/visibility.hpp"
#include <chrono>
#include <filesystem>
#include <ios>
#include <system_error>
#include <thread>

#if OS_IS_WINDOWS
#if RUNTIME_IS_MINGW
#include <sdkddkver.h>
#include <windows.h>
#elif RUNTIME_IS_MSVC
#include <SDKDDKVer.h>
#include <Windows.h>
#endif
#endif

#define OUTPUT_OBJECT_NAME "output"

namespace qat {

QatSitter* QatSitter::instance = nullptr;

QatSitter::QatSitter()
    : ctx(ir::Ctx::New()), Lexer(lexer::Lexer::get(ctx)), Parser(parser::Parser::get(ctx)),
      mainThread(std::this_thread::get_id()) {
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
	auto& log = Logger::get();
	log->diagnostic(
	    "Lexer speed   -> " +
	    std::to_string(
	        (u64)((((double)lexer::Lexer::lineCount) / ((double)lexer::Lexer::timeInMicroSeconds)) * 1000000.0)) +
	    " lines/s");
	log->diagnostic(
	    "Parser speed  -> " +
	    std::to_string(
	        (u64)((((double)lexer::Lexer::lineCount) / ((double)parser::Parser::timeInMicroSeconds)) * 1000000.0)) +
	    " lines/s & " +
	    std::to_string(
	        (u64)((((double)parser::Parser::tokenCount) / ((double)parser::Parser::timeInMicroSeconds)) * 1000000.0)) +
	    " tokens/s");
	if (ctx->clangAndLinkTimeInMs.has_value()) {
		log->diagnostic(
		    "Compile speed -> " +
		    std::to_string(
		        (u64)((((double)lexer::Lexer::lineCount) / ((double)ctx->qatCompileTimeInMs.value())) * 1000000.0)) +
		    " lines/s");
	}
	auto timeToString = [](u64 timeInMs) {
		if (timeInMs > 1000000) {
			return std::to_string(((double)timeInMs) / 1000000) + " s";
		} else if (timeInMs > 1000) {
			return std::to_string((double)timeInMs / 1000) + " ms";
		} else {
			return std::to_string(timeInMs) + " μs";
		}
	};
	log->diagnostic("Lexer time    -> " + timeToString(lexer::Lexer::timeInMicroSeconds));
	log->diagnostic("Parser time   -> " + timeToString(parser::Parser::timeInMicroSeconds));
	if (ctx->qatCompileTimeInMs.has_value() && ctx->clangAndLinkTimeInMs.has_value()) {
		log->diagnostic("Compile time  -> " + timeToString(ctx->qatCompileTimeInMs.value()));
		log->diagnostic("clang & lld   -> " + timeToString(ctx->clangAndLinkTimeInMs.value()));
	}
}

void QatSitter::initialise() {
	auto* config = cli::Config::get();
	auto& log    = Logger::get();
	for (const auto& path : config->get_paths()) {
		SHOW("Handling path for " << path)
		handle_path(path, ctx);
	}
	if (config->has_std_lib_path() && ctx->stdLibPossiblyRequired) {
		handle_path(config->get_std_lib_path(), ctx);
		if (ir::Mod::has_file_module(config->get_std_lib_path())) {
			ir::StdLib::stdLib = ir::Mod::get_file_module(config->get_std_lib_path());
		}
	}
	if (config->is_workflow_build() || config->is_workflow_analyse()) {
		auto qatStartTime = std::chrono::high_resolution_clock::now();
		for (auto* entity : fileEntities) {
			entity->node_handle_fs_brings(ctx);
		}
		for (auto* entity : fileEntities) {
			entity->node_create_modules(ctx);
		}
		for (auto* entity : fileEntities) {
			SHOW("Create Entity: " << entity->get_name())
			entity->node_create_entities(ctx);
		}
		for (auto* entity : fileEntities) {
			SHOW("Update Entity Dependencies: " << entity->get_name())
			entity->node_update_dependencies(ctx);
		}
		// for (auto* entity : fileEntities) {
		//   entity->handleBrings(ctx);
		// }
		// for (auto* entity : fileEntities) {
		//   entity->defineTypes(ctx);
		// }
		// for (auto* entity : fileEntities) {
		//   entity->defineNodes(ctx);
		// }
		// for (auto* entity : fileEntities) {
		//   entity->handleBrings(ctx);
		// }
		bool atleastOneEntityDone  = true;
		bool hasIncompleteEntities = true;
		while (hasIncompleteEntities && atleastOneEntityDone) {
			atleastOneEntityDone  = false;
			hasIncompleteEntities = false;
			for (auto* itMod : ir::Mod::allModules) {
				for (auto ent : itMod->entityEntries) {
					SHOW("Entity " << (ent->name.has_value() ? ("hasName: " + ent->name.value().value) : ""))
					SHOW("      type: " << ir::entity_type_to_string(ent->type))
					SHOW("      Is complete: " << (ent->are_all_phases_complete() ? "true" : "false"))
					SHOW("      Is ready: " << (ent->is_ready_for_next_phase() ? "true" : "false"))
					SHOW("      Iterations: " << ent->iterations)
					if (not ent->are_all_phases_complete()) {
						if (ent->is_ready_for_next_phase()) {
							ent->do_next_phase(itMod, ctx);
							atleastOneEntityDone = true;
						}
						if (not ent->are_all_phases_complete()) {
							hasIncompleteEntities = true;
						}
					}
					ent->iterations++;
				}
			}
		}
		if ((not atleastOneEntityDone) && hasIncompleteEntities) {
			Vec<ir::QatError> errors;
			for (auto* iterMod : ir::Mod::allModules) {
				for (auto ent : iterMod->entityEntries) {
					if (!ent->are_all_phases_complete()) {
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
									    +" at " + dep.entity->name.value().range.start_to_string() + "\n";
								} else {
									depStr += String(dep.type == ir::DependType::partial ? "- Depends partially on "
									                                                     : " - Depends on ") +
									          "unnamed " + ir::entity_type_to_string(ent->type) +
									          (dep.entity->astNode
									               ? (" at " + dep.entity->astNode->fileRange.start_to_string())
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
						    ent->astNode ? ent->astNode->fileRange
						                 : (ent->name.has_value() ? Maybe<FileRange>(ent->name.value().range) : None)));
					}
				}
			}
			ctx->Errors(errors);
		}
		for (auto* entity : fileEntities) {
			entity->setup_llvm_file(ctx);
		}
		// for (auto* entity : fileEntities) {
		//   auto* oldMod = ctx->setActiveModule(entity);
		//   entity->emitNodes(ctx);
		//   (void)ctx->setActiveModule(oldMod);
		// }
		SHOW("Emitted nodes")
		auto qatCompileTime = std::chrono::duration_cast<std::chrono::microseconds>(
		                          std::chrono::high_resolution_clock::now() - qatStartTime)
		                          .count();
		ctx->qatCompileTimeInMs = qatCompileTime;
		auto* cfg               = cli::Config::get();
		// if (cfg->hasOutputPath()) {
		//   fs::remove_all(cfg->getOutputPath() / "llvm");
		// }

		//
		//
		if (cfg->export_code_metadata()) {
			log->say("Exporting code metadata");
			SHOW("About to export code metadata")
			// NOLINTNEXTLINE(readability-isolate-declaration)
			Vec<JsonValue> modulesJSON, functionsJSON, prerunFunctionsJSON, genericFunctionsJSON, genericCoreTypesJSON,
			    structTypesJSON, mixTypesJSON, regionJSON, choiceJSON, typeDefinitionsJSON;
			for (auto* entity : fileEntities) {
				entity->output_all_overview(modulesJSON, functionsJSON, prerunFunctionsJSON, genericFunctionsJSON,
				                            genericCoreTypesJSON, structTypesJSON, mixTypesJSON, regionJSON, choiceJSON,
				                            typeDefinitionsJSON);
			}
			Vec<JsonValue> expressionUnits;
			for (auto* exp : ir::Value::allValues) {
				if (exp->get_ir_type() && exp->has_associated_range()) {
					Maybe<String> expStr;
					if (exp->is_prerun_value()) {
						expStr = exp->get_ir_type()->to_prerun_generic_string((ir::PrerunValue*)(exp));
					}
					expressionUnits.push_back(Json()
					                              ._("fileRange", exp->get_associated_range())
					                              ._("typeID", exp->get_ir_type()->get_id())
					                              ._("value", expStr.has_value() ? expStr.value() : JsonValue()));
				}
			}
			auto            codeStructFilePath = cfg->get_output_path() / "QatCodeInfo.json";
			bool            codeInfoExists     = fs::exists(codeStructFilePath);
			std::error_code errorCode;
			if (not codeInfoExists) {
				fs::create_directories(codeStructFilePath.parent_path(), errorCode);
			}
			if (codeInfoExists || (not errorCode)) {
				std::ofstream mStream;
				mStream.open(codeStructFilePath, std::ios_base::out | std::ios_base::trunc);
				if (mStream.is_open()) {
					mStream << Json()
					               ._("modules", modulesJSON)
					               ._("functions", functionsJSON)
					               ._("prerunFunctions", prerunFunctionsJSON)
					               ._("genericFunctions", genericFunctionsJSON)
					               ._("structTypes", structTypesJSON)
					               ._("genericStructTypes", genericCoreTypesJSON)
					               ._("mixTypes", mixTypesJSON)
					               ._("regions", regionJSON)
					               ._("choiceTypes", choiceJSON)
					               ._("typeDefinitions", typeDefinitionsJSON)
					               ._("expressionUnits", expressionUnits);
					mStream.close();
				} else {
					ctx->Error("Could not open the code info file for output", codeStructFilePath);
				}
			} else {
				ctx->Error("Could not create parent directory of the code info file", {codeStructFilePath});
			}
		}
		//
		//
		SHOW(cfg->should_export_ast())
		if (cfg->should_export_ast()) {
			for (auto* entity : fileEntities) {
				entity->export_json_from_ast(ctx);
			}
		}
		SHOW("Getting link start time")
		auto clear_llvm_files = [&] {
			if (cfg->clear_llvm()) {
				for (const auto& llPath : ctx->llvmOutputPaths) {
					fs::remove(llPath);
				}
				if (cfg->has_output_path() && fs::exists(cfg->get_output_path() / "llvm")) {
					fs::remove_all(cfg->get_output_path() / "llvm");
				}
			}
		};
		auto clangStartTime = std::chrono::high_resolution_clock::now();
		if (cfg->is_workflow_build()) {
			SHOW("Checking whether clang exists or not")
			if (check_executable_exists("clang") || check_executable_exists("clang++")) {
				SHOW("Executable paths count: " << ctx->executablePaths.size())
				for (auto* entity : fileEntities) {
					entity->compile_to_object(ctx);
				}
				SHOW("Compiled to object")
				ir::Mod::find_native_library_paths();
				SHOW("Handling native libs")
				for (auto* entity : fileEntities) {
					entity->handle_native_libs(ctx);
				}
				SHOW("Bundling modules")
				for (auto* entity : fileEntities) {
					entity->bundle_modules(ctx);
				}
				ctx->clangAndLinkTimeInMs = std::chrono::duration_cast<std::chrono::microseconds>(
				                                std::chrono::high_resolution_clock::now() - clangStartTime)
				                                .count();
				display_stats();
				ctx->write_json_result(true);
				clear_llvm_files();
				SHOW("Executable paths count: " << ctx->executablePaths.size())
				if (cfg->is_workflow_run() && not ctx->executablePaths.empty()) {
					if (llvm::Triple(cfg->get_target_triple()) != llvm::Triple(LLVM_HOST_TRIPLE)) {
						ctx->Error("The target provided for compilation is " + ctx->color(cfg->get_target_triple()) +
						               " which does not match the host target triplet of this compiler, which is " +
						               ctx->color(LLVM_HOST_TRIPLE) +
						               ". Cannot run built executables due to this mismatch",
						           None);
					}
					for (const auto& exePath : ctx->executablePaths) {
						SHOW("Running built executable at: " << exePath.string())
						auto cmdRes = run_command_get_output(fs::absolute(exePath).string(), {});
						std::cout << "\n===== Output of \"" + exePath.lexically_relative(fs::current_path()).string() +
						                 "\"\n" + cmdRes.second + "===== Status Code: " + std::to_string(cmdRes.first) +
						                 "\n";
						if (cmdRes.first) {
							std::cout << "\nThe built executable at " + ctx->color(exePath.string()) +
							                 " exited with error";
						}
					}
					SHOW("Ran all compiled executables")
				}
			} else {
				ctx->Error("Cannot find clang on path. Please make sure that you have clang with version 17 "
				           "or later installed and the path to clang is available in the system environment",
				           None);
			}
		} else {
			display_stats();
			clear_llvm_files();
			SHOW("clearLLVM called")
			ctx->write_json_result(true);
		}
	}
}

void QatSitter::remove_entity_with_path(const fs::path& path) {
	for (auto item = fileEntities.begin(); item != fileEntities.end(); item++) {
		if (((*item)->get_mod_type() == ir::ModuleType::file || (*item)->get_mod_type() == ir::ModuleType::folder) &&
		    fs::equivalent(fs::path((*item)->get_file_path()), path)) {
			fileEntities.erase(item);
			return;
		}
	}
}

Maybe<Pair<String, fs::path>> QatSitter::detect_lib_file(const fs::path& path) {
	if (fs::is_directory(path)) {
		SHOW("Path is a directory: " << fs::absolute(path).string())
		for (const auto& item : fs::directory_iterator(path)) {
			if (fs::is_regular_file(item)) {
				auto name = item.path().filename().string();
				if (name.ends_with(".lib.qat")) {
					SHOW("lib file detected: " << name.substr(0, name.length() - 8))
					return Pair<String, fs::path>(name.substr(0, name.length() - 8),
					                              item); // NOLINT(readability-magic-numbers)
				}
			}
		}
	} else if (fs::is_regular_file(path)) {
		SHOW("Path is a file: " << path.string())
		auto name = path.filename().string();
		if (name.ends_with(".lib.qat")) {
			SHOW("lib file detected: " << name.substr(0, name.length() - 8))
			return Pair<String, fs::path>(name.substr(0, name.length() - 8), path); // NOLINT(readability-magic-numbers)
		}
	}
	return None;
}

bool QatSitter::is_name_valid(const String& name) {
	auto lexRes = lexer::Lexer::word_to_token(name, nullptr);
	return (lexRes.has_value() && lexRes.value().type == lexer::TokenType::identifier);
}

void QatSitter::handle_path(const fs::path& mainPath, ir::Ctx* irCtx) {
	Vec<fs::path> broughtPaths;
	Vec<fs::path> memberPaths;
	auto*         cfg = cli::Config::get();
	SHOW("Handling path: " << mainPath.string())
	std::function<void(ir::Mod*, const fs::path&)> recursiveModuleCreator = [&](ir::Mod*        parentMod,
	                                                                            const fs::path& path) {
		for (auto const& item : fs::directory_iterator(path)) {
			if (fs::is_directory(item) && !fs::equivalent(item, cfg->get_output_path()) &&
			    !ir::Mod::has_folder_module(item)) {
				auto libCheckRes = detect_lib_file(item);
				if (libCheckRes.has_value()) {
					if (!is_name_valid(libCheckRes->first)) {
						irCtx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
						                 irCtx->color(libCheckRes->first) + " which is illegal",
						             None);
					}
					Lexer->change_file(fs::absolute(libCheckRes->second));
					Lexer->analyse();
					Parser->set_tokens(Lexer->get_tokens());
					auto parseRes(Parser->parse());
					for (const auto& bPath : Parser->get_brought_paths()) {
						broughtPaths.push_back(bPath);
					}
					for (const auto& mPath : Parser->get_member_paths()) {
						memberPaths.push_back(mPath);
					}
					Parser->clear_brought_paths();
					Parser->clear_member_paths();
					fileEntities.push_back(ir::Mod::create_root_lib(parentMod, fs::absolute(libCheckRes->second), path,
					                                                Identifier(libCheckRes->first, libCheckRes->second),
					                                                std::move(parseRes), VisibilityInfo::pub(), irCtx));
				} else {
					auto dirQatChecker = [](const fs::directory_entry& entry) {
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
						    Identifier(fs::absolute(item.path().filename()).string(), item.path()),
						    ir::ModuleType::folder, VisibilityInfo::pub(), irCtx);
						fileEntities.push_back(subfolder);
						recursiveModuleCreator(subfolder, item);
					} else {
						recursiveModuleCreator(parentMod, item);
					}
				}
			} else if (fs::is_regular_file(item) && !ir::Mod::has_file_module(item) &&
			           (item.path().extension() == ".qat")) {
				auto libCheckRes = detect_lib_file(item);
				if (libCheckRes.has_value() && !is_name_valid(libCheckRes.value().first)) {
					irCtx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
					                 irCtx->color(libCheckRes->first) + " which is illegal",
					             None);
				}
				Lexer->change_file(item.path().string());
				Lexer->analyse();
				Parser->set_tokens(Lexer->get_tokens());
				auto parseRes(Parser->parse());
				for (const auto& bPath : Parser->get_brought_paths()) {
					broughtPaths.push_back(bPath);
				}
				for (const auto& mPath : Parser->get_member_paths()) {
					memberPaths.push_back(mPath);
				}
				Parser->clear_brought_paths();
				Parser->clear_member_paths();
				if (libCheckRes.has_value()) {
					fileEntities.push_back(ir::Mod::create_root_lib(parentMod, fs::absolute(item), path,
					                                                Identifier(libCheckRes->first, libCheckRes->second),
					                                                std::move(parseRes), VisibilityInfo::pub(), irCtx));
				} else {
					fileEntities.push_back(ir::Mod::create_file_mod(
					    parentMod, fs::absolute(item), path, Identifier(item.path().filename().string(), item.path()),
					    std::move(parseRes), VisibilityInfo::pub(), irCtx));
				}
			}
		}
	};
	// FIXME - Check if modules are already part of another module
	SHOW("Created recursive module creator")
	if (fs::is_directory(mainPath) && !fs::equivalent(mainPath, cfg->get_output_path()) &&
	    !ir::Mod::has_folder_module(mainPath)) {
		SHOW("Is directory")
		auto libCheckRes = detect_lib_file(mainPath);
		if (libCheckRes.has_value()) {
			if (!is_name_valid(libCheckRes.value().first)) {
				irCtx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
				                 irCtx->color(libCheckRes->first) + " which is illegal",
				             None);
			}
			Lexer->change_file(libCheckRes->second);
			Lexer->analyse();
			Parser->set_tokens(Lexer->get_tokens());
			auto parseRes(Parser->parse());
			for (const auto& bPath : Parser->get_brought_paths()) {
				broughtPaths.push_back(bPath);
			}
			for (const auto& mPath : Parser->get_member_paths()) {
				memberPaths.push_back(mPath);
			}
			Parser->clear_brought_paths();
			Parser->clear_member_paths();
			fileEntities.push_back(ir::Mod::create_file_mod(nullptr, libCheckRes->second, mainPath,
			                                                Identifier(libCheckRes->first, libCheckRes->second),
			                                                std::move(parseRes), VisibilityInfo::pub(), irCtx));
		} else {
			auto* subfolder =
			    ir::Mod::create(Identifier(mainPath.filename().string(), mainPath), mainPath, mainPath.parent_path(),
			                    ir::ModuleType::folder, VisibilityInfo::pub(), irCtx);
			fileEntities.push_back(subfolder);
			recursiveModuleCreator(subfolder, mainPath);
		}
	} else if (fs::is_regular_file(mainPath) && !ir::Mod::has_file_module(mainPath)) {
		auto libCheckRes = detect_lib_file(mainPath);
		if (libCheckRes.has_value() && !is_name_valid(libCheckRes.value().first)) {
			irCtx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
			                 irCtx->color(libCheckRes->first) + " which is illegal",
			             None);
		}
		SHOW("Is regular file")
		Lexer->change_file(mainPath);
		Lexer->analyse();
		Parser->set_tokens(Lexer->get_tokens());
		auto parseRes(Parser->parse());
		for (const auto& bPath : Parser->get_brought_paths()) {
			broughtPaths.push_back(bPath);
		}
		for (const auto& mPath : Parser->get_member_paths()) {
			memberPaths.push_back(mPath);
		}
		Parser->clear_brought_paths();
		Parser->clear_member_paths();
		if (libCheckRes.has_value()) {
			fileEntities.push_back(ir::Mod::create_root_lib(nullptr, fs::absolute(mainPath), mainPath.parent_path(),
			                                                Identifier(libCheckRes->first, libCheckRes->second),
			                                                std::move(parseRes), VisibilityInfo::pub(), irCtx));
		} else {
			fileEntities.push_back(ir::Mod::create_file_mod(nullptr, fs::absolute(mainPath), mainPath.parent_path(),
			                                                Identifier(mainPath.filename().string(), mainPath),
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
	ir::Value::replace_uses_for_all();
	SHOW("Replaced uses for all llvm values")
	ir::Mod::clear_all();
	SHOW("Cleared all modules")
	ast::Node::clear_all();
	SHOW("Cleared all AST nodes")
	ast::Type::clear_all();
	SHOW("Cleared all AST types")
	ir::Value::clear_all();
	SHOW("Cleared all IR values")
	ir::Type::clear_all();
	SHOW("Cleared all IR types")
}

QatSitter::~QatSitter() { destroy(); }

} // namespace qat
