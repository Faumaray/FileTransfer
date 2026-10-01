#pragma once

namespace ftu::Void
{
	class Logger
	{
	public:
		Logger(const Logger&) = default;
		Logger(Logger&&) = delete;
		Logger& operator=(const Logger&) = default;
		Logger& operator=(Logger&&) = delete;
		virtual ~Logger() = default;

	private:
	};

} // namespace ftu::Void
