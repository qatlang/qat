#include "./create.hpp"
#include "../cli/logger.hpp"
#include "../lexer/lexer.hpp"
#include "../utils/find_executable.hpp"
#include "../utils/run_command.hpp"
#include <fstream>
#include <iostream>

#define AUTO_GENERATED_STUB                                                                                            \
	"/**   -----  QAT  -----\n"                                                                                        \
		<< "  * This is an auto-generated file.\n"                                                                     \
		<< "  * Do not use this for practical purposes,\n"                                                             \
		<< "  * and do not publish this as there is no\n"                                                              \
		<< "  * functionality here\n"                                                                                  \
		<< "  */\n\n"

namespace qat::cli {

void create_project(String name, fs::path path, bool isLib, Maybe<String> vcs) {
	auto& log	 = Logger::get();
	auto  lexRes = lexer::Lexer::word_to_token(name, nullptr);
	if (lexRes.has_value() && lexRes->type != lexer::TokenType::identifier) {
		log->fatalError(log->color(name) +
							" cannot be the name of the new project, as it is not a valid identifier in the language",
						None);
	}
	auto projDir = path / name;
	if (fs::exists(projDir)) {
		if (!fs::is_empty(projDir)) {
			log->fatalError("The path " + projDir.string() +
								" already exists and is not empty, so it cannot be used for the project",
							projDir);
		}
	} else {
		std::error_code err;
		fs::create_directories(projDir, err);
		if (err) {
			log->fatalError("Could not create the directory " + projDir.string() + ". The error is: " + err.message(),
							projDir);
		}
	}
	if (isLib) {
		auto		  libPath = projDir / (name + ".lib.qat");
		std::ofstream out(libPath);
		out << AUTO_GENERATED_STUB << "pub my_function() [\n"
			<< "\t// The following syntax allows you to have a function to be implemented later\n"
			<< "\tmeta:todo.\n"
			<< "]\n";
		out.close();
		std::cout << "✓ Created a qat library project in " + log->color(projDir.string()) + "\n";
	} else {
		auto		  mainPath = projDir / (name + ".qat");
		std::ofstream out(mainPath);
		out << AUTO_GENERATED_STUB << "pub main -> i32 [\n"
			<< "\tsay \"Hello, World!\".\n"
			<< "]\n";
		out.close();
		std::cout << "✓ Created a qat executable project in " + log->color(projDir.string()) + "\n";
	}
	if (vcs.has_value() && (vcs.value() == "git")) {
		auto gitPath = find_executable("git");
		if (not gitPath.has_value()) {
			log->fatalError(
				"Could not find git on PATH. Make sure that git is installed on your system. Could not initialise a git repository in the project directory " +
					projDir.string(),
				None);
		}
		auto checkRes = run_command_get_stderr(gitPath.value(), {"-C", projDir.string(), "rev-parse"});
		if (checkRes.first) {
			auto initRes = run_command_get_stderr(gitPath.value(), {"init", projDir.string()});
			if (initRes.first) {
				log->fatalError("Running " + log->color("git init " + projDir.string()) +
									" failed with the status code " + std::to_string(initRes.first) +
									" and error message: " + initRes.second,
								projDir);
			} else {
				std::cout << "✓ Successfully initialised a git repository" << std::endl;
			}

		} else {
			log->warn("The project directory " + projDir.string() +
						  " is found to be part of a git repository, so no new repository has been created",
					  None);
		}
	}
}

} // namespace qat::cli
