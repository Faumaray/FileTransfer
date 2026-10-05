#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace ftu::transfer
{
	struct Options
	{
		std::filesystem::path output_directory;
		std::string socket_path;
		std::size_t worker_count = 4;
		std::size_t max_connections = 128;
		std::uint64_t max_file_size = 0;
		std::chrono::milliseconds inactivity_timeout {120'000};
		unsigned retry_count = 8;
		static Options fromEnvironment();
	};

} // namespace ftu::transfer
