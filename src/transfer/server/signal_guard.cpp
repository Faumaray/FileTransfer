#include "ftu/transfer/server/signal_guard.hpp"
#include "ftu/platform/system_error.hpp"

namespace ftu::transfer::server
{
	SignalGuard::SignalGuard()
	{
		::sigemptyset(&m_mask);
		::sigaddset(&m_mask, SIGINT);
		::sigaddset(&m_mask, SIGTERM);
		platform::check(::sigprocmask(SIG_BLOCK, &m_mask, &m_previous), "sigprocmask");
	}

	SignalGuard::~SignalGuard()
	{
		::sigprocmask(SIG_SETMASK, &m_previous, nullptr);
	}
} // namespace ftu::transfer::server
