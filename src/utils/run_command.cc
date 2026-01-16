#include "./run_command.hpp"
#include "../cli/logger.hpp"

#include <boost/asio/readable_pipe.hpp>
#include <boost/asio/writable_pipe.hpp>
#include <boost/process/stdio.hpp>
#include <boost/process/v2/process.hpp>
#include <helpers/array.hpp>

namespace qat {

int run_command_get_code(String command, Vec<String> const& args) {
	boost::asio::io_context    ctx;
	boost::process::process    proc(ctx, command, args,
	                                boost::process::process_stdio{
	                                    .in  = nullptr,
	                                    .out = nullptr,
	                                    .err = nullptr,
                                 });
	boost::process::error_code ec;
	auto                       exitCode = proc.wait(ec);
	if (ec.failed()) {
		String message(command);
		for (auto& arg : args) {
			message += " " + arg;
		}
		auto& log = Logger::get();
		log->fatalError("Failed waiting for the following command to finish. Could not retrieve the exit code.\n" +
		                    log->color(message),
		                None);
	}
	return exitCode;
}

Pair<int, String> run_command_get_stdout(String command, Vec<String> const& args) {
	boost::asio::io_context    ctx;
	boost::asio::readable_pipe pOut(ctx);
	boost::process::process    proc(ctx, command, args,
	                                boost::process::process_stdio{
	                                    .in  = nullptr,
	                                    .out = pOut,
	                                    .err = nullptr,
                                 });
	String                     result;
	Array<char, 2048>          buffer{};
	boost::system::error_code  sysErr;
	while (true) {
		auto size = pOut.read_some(boost::asio::buffer(buffer), sysErr);
		if (sysErr == boost::asio::error::eof) {
			break;
		}
		if (sysErr) {
			String message(command);
			for (auto& arg : args) {
				command += " " + arg;
			}
			auto& log = Logger::get();
			log->fatalError("Failed to read the standard output of the following command:\n" + log->color(message),
			                None);
		}
		result.append(buffer.data(), size);
	}
	boost::process::error_code err;
	auto                       exitCode = proc.wait(err);
	if (err.failed()) {
		String message(command);
		for (auto& arg : args) {
			command += " " + arg;
		}
		auto& log = Logger::get();
		log->fatalError("Failed to wait for the following command to finish execution:\n" + log->color(message), None);
	}
	return {exitCode, result};
}

int run_command_with_output(String command, Vec<String> const& args) {
	boost::asio::io_context    ctx;
	boost::process::process    proc(ctx, command, args, boost::process::process_stdio{});
	boost::process::error_code err;
	auto                       exitCode = proc.wait(err);
	if (err.failed()) {
		String message(command);
		for (auto& arg : args) {
			command += " " + arg;
		}
		auto& log = Logger::get();
		log->fatalError("Failed to wait for the following command to finish execution:\n" + log->color(message), None);
	}
	return exitCode;
}

Pair<int, String> run_command_get_output(String command, Vec<String> const& args) {
	boost::asio::io_context    ctx;
	boost::asio::readable_pipe readPipe(ctx);
	boost::asio::writable_pipe writePipe(ctx);
	boost::asio::connect_pipe(readPipe, writePipe);
	boost::process::process proc(ctx, command, args,
	                             boost::process::process_stdio{
	                                 .in  = nullptr,
	                                 .out = writePipe,
	                                 .err = writePipe,
	                             });
	writePipe.close();
	String                    result;
	Array<char, 2048>         buffer;
	boost::system::error_code sysErr;
	while (true) {
		auto size = readPipe.read_some(boost::asio::buffer(buffer), sysErr);
		if (sysErr == boost::asio::error::eof) {
			break;
		}
		if (sysErr) {
			String message(command);
			for (auto& arg : args) {
				command += " " + arg;
			}
			auto& log = Logger::get();
			log->fatalError("Failed to read the output of the following command:\n" + log->color(message), None);
		}
		result.append(buffer.data(), size);
	}
	boost::process::error_code err;
	auto                       exitCode = proc.wait(err);
	if (err.failed()) {
		String message(command);
		for (auto& arg : args) {
			command += " " + arg;
		}
		auto& log = Logger::get();
		log->fatalError("Failed to wait for the following command to finish execution:\n" + log->color(message), None);
	}

	return Pair<int, String>(exitCode, result);
}

Pair<int, String> run_command_get_stderr(String command, Vec<String> const& args) {
	boost::asio::io_context    ctx;
	boost::asio::readable_pipe pErr(ctx);
	boost::process::process    proc(ctx, command, args,
	                                boost::process::process_stdio{
	                                    .in  = nullptr,
	                                    .out = nullptr,
	                                    .err = pErr,
                                 });
	String                     result;
	Array<char, 2048>          buffer{};
	boost::system::error_code  sysErr;
	while (true) {
		const auto size = pErr.read_some(boost::asio::buffer(buffer), sysErr);
		if (sysErr == boost::asio::error::eof) {
			break;
		}
		if (sysErr) {
			String message(command);
			for (auto& arg : args) {
				command += " " + arg;
			}
			auto& log = Logger::get();
			log->fatalError(
			    "Failed to read the standard error output of the following command:\n" + log->color(message), None);
		}
		result.append(buffer.data(), size);
	}
	boost::process::error_code err;
	auto                       exitCode = proc.wait(err);
	if (err.failed()) {
		String message(command);
		for (auto& arg : args) {
			command += " " + arg;
		}
		auto& log = Logger::get();
		log->fatalError("Failed to wait for the following command to finish execution:\n" + log->color(message), None);
	}
	return {exitCode, result};
}

Many<int, String, String> run_command_get_stdout_and_stderr(String command, Vec<String> const& args) {
	boost::asio::io_context    ctx;
	boost::asio::readable_pipe out_read(ctx);
	boost::asio::writable_pipe out_write(ctx);
	boost::asio::connect_pipe(out_read, out_write);
	boost::asio::readable_pipe err_read(ctx);
	boost::asio::writable_pipe err_write(ctx);
	boost::asio::connect_pipe(err_read, err_write);
	auto proc = boost::process::process(ctx, command, args,
	                                    boost::process::process_stdio{
	                                        .in  = nullptr,
	                                        .out = out_write,
	                                        .err = err_write,
	                                    });
	out_write.close();
	err_write.close();
	auto read_from_pipe = [&](boost::asio::readable_pipe& pipe) -> String {
		String                    res;
		Array<char, 2048>         buffer;
		boost::system::error_code ec;
		while (true) {
			const auto size = pipe.read_some(boost::asio::buffer(buffer), ec);
			if (ec == boost::asio::error::eof) {
				break;
			} else if (ec.failed()) {
				String message(command);
				for (auto& arg : args) {
					message += " " + arg;
				}
				auto& log = Logger::get();
				log->fatalError("Failed to read output for the following command:\n" + log->color(message), None);
			}
			res.append(buffer.data(), size);
		}
		return res;
	};

	String                     stdOut = read_from_pipe(out_read);
	String                     stdErr = read_from_pipe(err_read);
	boost::process::error_code err;
	const auto                 exitCode = proc.wait(err);
	if (err.failed()) {
		String message(command);
		for (auto& arg : args) {
			command += " " + arg;
		}
		auto& log = Logger::get();
		log->fatalError("Failed to wait for the following command to finish execution:\n" + log->color(message), None);
	}
	return std::make_tuple(exitCode, stdOut, stdErr);
}

} // namespace qat
