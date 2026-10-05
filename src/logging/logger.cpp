#include "ftu/logging/logger.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>

namespace ftu::logging
{
	void Logger::print(Level level, const std::string& message)
	{
		static constexpr std::array<std::string_view, 4> NAMES {"TRACE", "INFO", "WARNING", "ERROR"};
		static constexpr std::string_view DIGITS = "0123456789abcdef";
		const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm utc {};
		::gmtime_r(&now, &utc);
		std::array<char, 32> stamp {};
		std::string line(
			stamp.data(), std::strftime(stamp.data(), stamp.size(), "%Y-%m-%dT%H:%M:%SZ ", &utc)
		);
		line += NAMES.at(static_cast<std::size_t>(level));
		line += ' ';
		for (const char character : message)
		{
			const auto byte = static_cast<unsigned char>(character);
			if (byte < 0x20 || byte == 0x7F)
			{
				line += "\\x";
				line += DIGITS[byte >> 4U];
				line += DIGITS[byte & 0xFU];
			}
			else
			{
				line += character;
			}
		}
		line += '\n';
		std::FILE* stream = level == Level::Warning || level == Level::Error ? stderr : stdout;
		std::fwrite(line.data(), 1, line.size(), stream);
		std::fflush(stream);
	}
} // namespace ftu::logging
