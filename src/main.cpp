#include <ftu/logging/logger.hpp>
#include <ftu/transfer/client.hpp>
#include <ftu/transfer/server.hpp>
#include <ftu/transfer/transport/unix_socket/connector.hpp>
#include <ftu/transfer/transport/unix_socket/listener.hpp>

#include <iostream>

namespace
{
	// FIXME(Global): remove exceptions and move Result types, upgrade gcc version
	namespace transfer = ftu::transfer;
	namespace unix_socket = ftu::transfer::transport::unix_socket;

	void usage(std::string_view program)
	{
		std::cout << "Usage:\n  " << program << " -s\n  " << program
				  << " -c <file>\n\n"
					 "Linux Unix-socket file transfer with CRC-64/ECMA-182 and resumable UUIDv8 sessions.\n"
					 "Received bytes are saved unchanged as {date}_{time}.hex next to the executable.\n"
					 "Environment: FTU_SOCKET, FTU_WORKERS, FTU_MAX_CLIENTS, FTU_MAX_FILE_SIZE,\n"
					 "             FTU_TIMEOUT_MS, FTU_RETRIES.\n";
	}
} // namespace

int main(int argc, char* argv[])
{
	if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"))
	{
		usage(argv[0]);
		return 0;
	}
	const bool serverMode = argc == 2 && std::string(argv[1]) == "-s";
	const bool clientMode = argc == 3 && (std::string(argv[1]) == "-c" || std::string(argv[1]) == "-с");
	if (!serverMode && !clientMode)
	{
		usage(argv[0]);
		return 2;
	}
	try
	{
		const auto options = transfer::Options::fromEnvironment();
		if (serverMode)
		{
			transfer::storage::SessionStore store(options.output_directory, options.max_file_size);
			transfer::Server server(
				std::make_unique<unix_socket::Listener>(options.socket_path), store, options
			);
			server.run();
		}
		else
		{
			transfer::Client client(
				std::make_unique<unix_socket::Connector>(options.socket_path), options, argv[2]
			);
			client.run();
		}
		return 0;
	}
	catch (const std::exception& error)
	{
		ftu::logging::Logger::error("\"{}\"", error.what());
		return 1;
	}
}
