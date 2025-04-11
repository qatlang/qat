#include "./run_command.hpp"

#include <boost/process/child.hpp>
#include <boost/process/environment.hpp>
#include <boost/process/io.hpp>

#include <iostream>

namespace qat {

int run_command_get_code(String command, Vec<String> const& args) {
	boost::process::ipstream outStream;
	boost::process::child    childProcess(command, args, boost::process::std_out > boost::process::null);
	childProcess.wait();
	return childProcess.exit_code();
}

Pair<int, String> run_command_get_stdout(String command, Vec<String> const& args) {
	boost::process::ipstream outStream;
	boost::process::child    childProcess(command, args, boost::process::std_out > outStream);
	childProcess.wait();
	String output;
	for (String line; std::getline(outStream, line);) {
		output += line + "\n";
	}
	return Pair<int, String>(childProcess.exit_code(), output);
}

int run_command_with_output(String command, Vec<String> const& args) {
	boost::process::child childProcess(command, args);
	childProcess.wait();
	return childProcess.exit_code();
}

Pair<int, String> run_command_get_output(String command, Vec<String> const& args) {
	boost::process::ipstream outStream;
	boost::process::child childProcess(command, args, (boost::process::std_err & boost::process::std_out) > outStream);
	childProcess.wait();
	String output;
	for (String line; std::getline(outStream, line);) {
		output += line + "\n";
	}
	return Pair<int, String>(childProcess.exit_code(), output);
}

Pair<int, String> run_command_get_stderr(String command, Vec<String> const& args) {
	boost::process::ipstream errorStream;
	boost::process::child    childProcess(command, args, boost::process::std_err > errorStream);
	childProcess.wait();
	String errorOutput;
	for (String line; not std::getline(errorStream, line).eof();) {
		errorOutput += line + "\n";
	}
	return Pair<int, String>(childProcess.exit_code(), errorOutput);
}

std::tuple<int, String, String> run_command_get_stdout_and_stderr(String command, Vec<String> const& args) {
	boost::process::ipstream errorStream;
	boost::process::ipstream outStream;
	boost::process::child    childProcess(command, args, boost::process::std_out > outStream,
	                                      boost::process::std_err > errorStream);
	childProcess.wait();
	String output;
	String errorOutput;
	for (String line; std::getline(outStream, line);) {
		output += line + "\n";
	}
	for (String line; std::getline(errorStream, line);) {
		errorOutput += line + "\n";
	}
	return std::tuple<int, String, String>(childProcess.exit_code(), output, errorOutput);
}

} // namespace qat
