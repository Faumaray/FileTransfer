#include "ftu/transfer/options.hpp"
#include "ftu/platform/file_descriptor.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <limits>
#include <stdexcept>
#include <thread>
#include <unistd.h>

namespace ftu::transfer
{
	namespace
	{
		std::uint64_t setting(const char* name, std::uint64_t fallback, std::uint64_t min, std::uint64_t max)
		{
			const char* text = std::getenv(name);
			if (text == nullptr)
			{
				return fallback;
			}
			const std::string value(text);
			std::uint64_t result = 0;
			const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
			if (parsed.ec != std::errc {} || parsed.ptr != value.data() + value.size() || result < min ||
				result > max)
			{
				throw std::runtime_error(std::string("invalid environment setting ") + name);
			}
			return result;
		}

		std::string defaultSocketPath()
		{
			const std::filesystem::path directory = "/tmp/FTU-" + std::to_string(::geteuid());
			if (std::filesystem::create_directory(directory))
			{
				std::filesystem::permissions(directory, std::filesystem::perms::owner_all);
			}
			return (directory / "transfer.sock").string();
		}
	} // namespace

	Options Options::fromEnvironment()
	{
		Options options;
		options.output_directory = std::filesystem::read_symlink("/proc/self/exe").parent_path();
		const char* configured = std::getenv("FTU_SOCKET");
		options.socket_path = configured != nullptr ? std::string(configured) : defaultSocketPath();
		const std::filesystem::path socket = options.socket_path;
		if (!socket.is_absolute() || !socket.has_filename())
		{
			throw std::runtime_error("FTU_SOCKET must be an absolute socket pathname");
		}
		const auto parent =
			platform::FileDescriptor::open(socket.parent_path(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
		if (!parent.ownedPrivately())
		{
			throw std::runtime_error("socket parent must be owned by current user and have mode 0700");
		}
		const auto hardware = std::clamp(std::thread::hardware_concurrency(), 2U, 8U);
		options.worker_count = static_cast<std::size_t>(setting("FTU_WORKERS", hardware, 1, 64));
		options.max_connections = static_cast<std::size_t>(setting("FTU_MAX_CLIENTS", 128, 1, 4096));
		options.max_file_size =
			setting("FTU_MAX_FILE_SIZE", 0, 0, static_cast<std::uint64_t>(std::numeric_limits<off_t>::max()));
		options.inactivity_timeout =
			std::chrono::milliseconds(setting("FTU_TIMEOUT_MS", 120'000, 100, 86'400'000));
		options.retry_count = static_cast<unsigned>(setting("FTU_RETRIES", 8, 0, 100));
		return options;
	}
} // namespace ftu::transfer
