#include "./sitter.hpp"
#include "./IR/stdlib.hpp"
#include "./IR/type_id.hpp"
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
	SHOW("Module count: " << ir::Mod::allModules.size())
	if (config->is_workflow_build() || config->is_workflow_analyse()) {
		auto qatStartTime = std::chrono::high_resolution_clock::now();
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
		ir::TypeInfo::finalise_type_infos(ctx);
		for (auto* entity : fileEntities) {
			entity->setup_llvm_file(ctx);
		}
		auto qatCompileTime = std::chrono::duration_cast<std::chrono::microseconds>(
		                          std::chrono::high_resolution_clock::now() - qatStartTime)
		                          .count();
		ctx->qatCompileTimeInMs = qatCompileTime;
		auto* cfg               = cli::Config::get();
		// if (cfg->export_code_metadata()) {
		// 	log->say("Exporting code metadata");
		// 	Vec<JsonValue> modulesJSON, functionsJSON, prerunFunctionsJSON, genericFunctionsJSON,
		// 	    genericStructTypesJSON, structTypesJSON, mixTypesJSON, regionJSON, choiceJSON, typeDefinitionsJSON,
		// 	    genericTypeDefsJSON, skillsJSON, genericSkillsJSON;
		// 	for (auto* entity : fileEntities) {
		// 		entity->output_all_overview(modulesJSON, functionsJSON, prerunFunctionsJSON, genericFunctionsJSON,
		// 		                            genericStructTypesJSON, structTypesJSON, mixTypesJSON, regionJSON,
		// 		                            choiceJSON, typeDefinitionsJSON, genericTypeDefsJSON, skillsJSON,
		// 		                            genericSkillsJSON);
		// 	}
		// 	log->say("Exporting expression units");
		// 	Vec<JsonValue> expressionUnits;
		// 	for (auto* exp : ir::Value::allValues) {
		// 		if (exp->get_ir_type() && exp->has_associated_range()) {
		// 			Maybe<String> expStr;
		// 			if (exp->is_prerun_value()) {
		// 				expStr = exp->get_ir_type()->to_prerun_generic_string((ir::PrerunValue*)(exp));
		// 			}
		// 			expressionUnits.push_back(Json()
		// 			                              ._("fileRange", exp->get_associated_range())
		// 			                              ._("typeID", exp->get_ir_type()->get_id())
		// 			                              ._("value", expStr.has_value() ? expStr.value() : JsonValue()));
		// 		}
		// 	}
		// 	auto            codeStructFilePath = cfg->get_output_path() / "QatCodeInfo.json";
		// 	bool            codeInfoExists     = fs::exists(codeStructFilePath);
		// 	std::error_code errorCode;
		// 	if (not codeInfoExists) {
		// 		fs::create_directories(codeStructFilePath.parent_path(), errorCode);
		// 	}
		// 	if (not codeInfoExists && errorCode) {
		// 		ctx->Error("Could not create parent directory of the code info file. The error is " +
		// 		               errorCode.message(),
		// 		           {codeStructFilePath});
		// 	}
		// 	std::ofstream mStream;
		// 	mStream.open(codeStructFilePath, std::ios_base::out | std::ios_base::trunc);
		// 	if (not mStream.is_open()) {
		// 		ctx->Error("Could not open the code info file for output", codeStructFilePath);
		// 	}
		// 	log->say("Writing code info file");
		// 	mStream << Json()
		// 	               ._("modules", modulesJSON)
		// 	               ._("functions", functionsJSON)
		// 	               ._("prerunFunctions", prerunFunctionsJSON)
		// 	               ._("genericFunctions", genericFunctionsJSON)
		// 	               ._("structTypes", structTypesJSON)
		// 	               ._("genericStructTypes", genericStructTypesJSON)
		// 	               ._("mixTypes", mixTypesJSON)
		// 	               ._("regions", regionJSON)
		// 	               ._("choiceTypes", choiceJSON)
		// 	               ._("typeDefinitions", typeDefinitionsJSON)
		// 	               ._("genericTypeDefinitions", genericTypeDefsJSON)
		// 	               ._("skills", skillsJSON)
		// 	               ._("genericSkills", genericSkillsJSON)
		// 	               ._("expressionUnits", expressionUnits);
		// 	log->say("Wrote code info JSON");
		// 	mStream.close();
		// }
		//
		//
		log->say("Checked AST export");
		if (cfg->should_export_ast()) {
			log->say("Exporting AST representation");
			for (auto* entity : fileEntities) {
				entity->export_json_from_ast(ctx);
			}
		}
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
			if (cfg->has_clang_path() || check_executable_exists("qat-clang") || check_executable_exists("clang") ||
			    check_executable_exists("clang++") || check_executable_exists("clang-20") ||
			    check_executable_exists("clang++-20") || check_executable_exists("clang-19") ||
			    check_executable_exists("clang++-19") || check_executable_exists("clang-18") ||
			    check_executable_exists("clang++-18") || check_executable_exists("clang-17") ||
			    check_executable_exists("clang++-17")) {
				for (auto* entity : fileEntities) {
					entity->compile_to_object(ctx);
				}
				ir::Mod::find_native_library_paths();
				for (auto* entity : fileEntities) {
					entity->handle_native_libs(ctx);
				}
				for (auto* entity : fileEntities) {
					entity->bundle_modules(ctx);
				}
				ctx->clangAndLinkTimeInMs = std::chrono::duration_cast<std::chrono::microseconds>(
				                                std::chrono::high_resolution_clock::now() - clangStartTime)
				                                .count();
				display_stats();
				SHOW("Displayed stats")
				ctx->write_json_result(true);
				SHOW("Wrote JSON result")
				clear_llvm_files();
				SHOW("Cleared llvm files")
				log->say("Cleared LLVM files");
				if (cfg->is_workflow_run() && not ctx->executablePaths.empty()) {
					if (llvm::Triple(cfg->get_target_triple()) != llvm::Triple(LLVM_HOST_TRIPLE)) {
						ctx->Error("The target provided for compilation is " + ctx->color(cfg->get_target_triple()) +
						               " which does not match the host target triplet of this compiler, which is " +
						               ctx->color(LLVM_HOST_TRIPLE) +
						               ". Cannot run built executables due to this mismatch",
						           None);
					}
					for (const auto& exePath : ctx->executablePaths) {
						std::cout << "\n===== Output of \"" << exePath.lexically_relative(fs::current_path()).string()
						          << "\"\n";
						auto exitCode = run_command_with_output(fs::absolute(exePath).string(), {});
						std::cout << "\n===== Status Code: " << std::to_string(exitCode) << "\n";
						if (exitCode) {
							std::cout << "\nThe built executable at " + ctx->color(exePath.string()) +
							                 " exited with error";
						}
					}
				}
				SHOW("Workflow run check complete")
			} else {
				ctx->Error(
				    "Cannot find clang on path. Please make sure that you have clang with version 17 "
				    "or later installed and the path to clang executable is present in the system PATH environment variable. Or else, provide path to a valid version of clang using the command line argument " +
				        ctx->color("--clang=/path/to/clang/exe"),
				    None);
			}
		} else {
			display_stats();
			clear_llvm_files();
			ctx->write_json_result(true);
		}
	}
	SHOW("Initialise complete")
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
		for (const auto& item : fs::directory_iterator(path)) {
			if (fs::is_regular_file(item)) {
				auto name = item.path().filename().string();
				if (name.ends_with(".lib.qat")) {
					return Pair<String, fs::path>(name.substr(0, name.length() - 8),
					                              item); // NOLINT(readability-magic-numbers)
				}
			}
		}
	} else if (fs::is_regular_file(path)) {
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
	Vec<fs::path>                                  broughtPaths;
	Vec<fs::path>                                  memberPaths;
	auto*                                          cfg                    = cli::Config::get();
	std::function<void(ir::Mod*, const fs::path&)> recursiveModuleCreator = [&](ir::Mod*        parentMod,
	                                                                            const fs::path& path) {
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
	} else if (fs::is_regular_file(mainPath) && not ir::Mod::has_file_module(mainPath)) {
		auto libCheckRes = detect_lib_file(mainPath);
		if (libCheckRes.has_value() && not is_name_valid(libCheckRes.value().first)) {
			irCtx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
			                 irCtx->color(libCheckRes->first) + " which is illegal",
			             None);
		}
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
