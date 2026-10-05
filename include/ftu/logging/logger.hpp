#pragma once

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace ftu::logging
{
	class Logger
	{
	public:
		Logger() = delete;

		template <class... ARGS>
		static void trace(std::string_view pattern, const ARGS&... args) noexcept
		{
			write(Level::Trace, pattern, args...);
		}

		template <class... ARGS>
		static void info(std::string_view pattern, const ARGS&... args) noexcept
		{
			write(Level::Info, pattern, args...);
		}

		template <class... ARGS>
		static void warn(std::string_view pattern, const ARGS&... args) noexcept
		{
			write(Level::Warning, pattern, args...);
		}

		template <class... ARGS>
		static void error(std::string_view pattern, const ARGS&... args) noexcept
		{
			write(Level::Error, pattern, args...);
		}

	private:
		enum class Level : std::uint8_t
		{
			Trace,
			Info,
			Warning,
			Error
		};

		template <class... ARGS>
		static void write(Level level, std::string_view pattern, const ARGS&... args) noexcept
		{
			try
			{
				std::ostringstream message;
				(substitute(message, pattern, args), ...);
				message << pattern;
				print(level, message.str());
			}
			catch (...)
			{
			}
		}

		template <class T>
		static void substitute(std::ostringstream& out, std::string_view& pattern, const T& value)
		{
			const auto open = pattern.find('{');
			const auto close = pattern.find('}', open);
			if (close == std::string_view::npos)
			{
				return;
			}
			out << pattern.substr(0, open);
			const bool hex = pattern.substr(open, close + 1 - open) == "{:x}";
			pattern.remove_prefix(close + 1);
			if constexpr (std::is_unsigned_v<T>)
			{
				if (hex)
				{
					out << std::hex << std::setfill('0') << std::setw(sizeof(T) * 2)
						<< static_cast<std::uint64_t>(value) << std::dec;
					return;
				}
			}
			out << value;
		}

		static void print(Level level, const std::string& message);
	};
} // namespace ftu::logging
