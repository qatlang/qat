#include "./run_command.hpp"
#include <subprocess/subprocess.h>

namespace qat {

Maybe<int> run_command_get_code(String command, Vec<String> args) {
	Vec<const char*> cmdLine(args.size() + 1);
	cmdLine.push_back(command.c_str());
	for (auto& it : args) {
		cmdLine.push_back(it.c_str());
	}
	subprocess_s subProc;
	int			 result = subprocess_create(cmdLine.data(), subprocess_option_inherit_environment, &subProc);
	if (result != 0) {
		return None;
	}
	subprocess_join(&subProc, &result);
	subprocess_destroy(&subProc);
	return result;
}

Maybe<Pair<int, String>> run_command_get_stdout(String command, Vec<String> args) {
	Vec<const char*> cmdLine(args.size() + 1);
	cmdLine.push_back(command.c_str());
	for (auto& it : args) {
		cmdLine.push_back(it.c_str());
	}
	subprocess_s subProc;
	int			 result = subprocess_create(cmdLine.data(), subprocess_option_inherit_environment, &subProc);
	if (result != 0) {
		return None;
	}
	auto progOut = subprocess_stdout(&subProc);
	subprocess_join(&subProc, &result);
	String outRes;
	char   outBuffer[128] = {0};
	while (std::fgets(outBuffer, sizeof(outBuffer), progOut) != nullptr) {
		outRes += &outBuffer[0];
	}
	subprocess_destroy(&subProc);
	return Pair<int, String>(result, outRes);
}

Maybe<Pair<int, String>> run_command_get_output(String command, Vec<String> args) {
	Vec<const char*> cmdLine(args.size() + 1);
	cmdLine.push_back(command.c_str());
	for (auto& it : args) {
		cmdLine.push_back(it.c_str());
	}
	subprocess_s subProc;
	int			 result = subprocess_create(
		 cmdLine.data(), subprocess_option_inherit_environment | subprocess_option_combined_stdout_stderr, &subProc);
	if (result != 0) {
		return None;
	}
	auto progOut = subprocess_stdout(&subProc);
	subprocess_join(&subProc, &result);
	String outRes;
	char   outBuffer[128] = {0};
	while (std::fgets(outBuffer, sizeof(outBuffer), progOut) != nullptr) {
		outRes += &outBuffer[0];
	}
	subprocess_destroy(&subProc);
	return Pair<int, String>(result, outRes);
}

Maybe<Pair<int, String>> run_command_get_stderr(String command, Vec<String> args) {
	Vec<const char*> cmdLine(args.size() + 1);
	cmdLine.push_back(command.c_str());
	for (auto& it : args) {
		cmdLine.push_back(it.c_str());
	}
	subprocess_s subProc;
	int			 result = subprocess_create(
		 cmdLine.data(), subprocess_option_inherit_environment | subprocess_option_combined_stdout_stderr, &subProc);
	if (result != 0) {
		return None;
	}
	auto progErr = subprocess_stderr(&subProc);
	subprocess_join(&subProc, &result);
	String errRes;
	char   errBuffer[128] = {0};
	while (std::fgets(errBuffer, sizeof(errBuffer), progErr) != nullptr) {
		errRes += &errBuffer[0];
	}
	subprocess_destroy(&subProc);
	return Pair<int, String>(result, errRes);
}

Maybe<std::tuple<int, String, String>> run_command_get_stdout_and_stderr(String command, Vec<String> args) {
	Vec<const char*> cmdLine(args.size() + 1);
	cmdLine.push_back(command.c_str());
	for (auto& it : args) {
		cmdLine.push_back(it.c_str());
	}
	subprocess_s subProc;
	int			 result = subprocess_create(
		 cmdLine.data(), subprocess_option_inherit_environment | subprocess_option_combined_stdout_stderr, &subProc);
	if (result != 0) {
		return None;
	}
	auto progOut = subprocess_stdout(&subProc);
	auto progErr = subprocess_stderr(&subProc);
	subprocess_join(&subProc, &result);
	String outRes;
	char   outBuffer[128] = {0};
	while (std::fgets(outBuffer, sizeof(outBuffer), progOut) != nullptr) {
		outRes += &outBuffer[0];
	}
	String errRes;
	char   errBuffer[128] = {0};
	while (std::fgets(errBuffer, sizeof(errBuffer), progErr) != nullptr) {
		errRes += &errBuffer[0];
	}
	subprocess_destroy(&subProc);
	return std::tuple<int, String, String>(result, outRes, errRes);
}

} // namespace qat
