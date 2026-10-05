#include <ftu/logging/logger.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <sstream>

namespace ftu::logging
{
	void Logger::print(Level level, const std::string& message)
	{
		static constexpr std::array<std::string_view, 4> NAMES {"TRACE", "INFO", "WARNING", "ERROR"};
		const std::time_t now =
			std::chrono::high_resolution_clock::to_time_t(std::chrono::high_resolution_clock::now());
		std::stringstream ss {};
		std::tm utc {};
		if (::gmtime_r(&now, &utc) == nullptr)
		{
			ss << now;
		}

		ss << NAMES.at(static_cast<std::size_t>(level)) << ": " << message;
		auto& stream = level == Level::Warning || level == Level::Error ? std::cerr : std::cout;
		stream.imbue(std::locale::classic());
		stream << std::put_time(&utc, "%Y-%m-%d %H:%M:%S; ") << ss.view() << "\n";
		std::flush(stream);
	}
} // namespace ftu::logging
