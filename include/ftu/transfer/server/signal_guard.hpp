#pragma once

#include <csignal>

namespace ftu::transfer::server
{
	class SignalGuard
	{
	public:
		SignalGuard();
		~SignalGuard();
		SignalGuard(const SignalGuard&) = delete;
		SignalGuard& operator=(const SignalGuard&) = delete;

		[[nodiscard]] sigset_t mask() const noexcept { return m_mask; }

	private:
		sigset_t m_mask {};
		sigset_t m_previous {};
	};
} // namespace ftu::transfer::server
