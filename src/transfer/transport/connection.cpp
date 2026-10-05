#include "ftu/transfer/transport/connection.hpp"
#include "ftu/platform/system_error.hpp"
#include "ftu/transfer/transport/disconnected.hpp"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <poll.h>

namespace ftu::transfer::transport
{
	void Connection::waitReady(std::int16_t events, std::chrono::steady_clock::time_point deadline) const
	{
		for (;;)
		{
			const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
				deadline - std::chrono::steady_clock::now()
			);
			if (remaining.count() <= 0)
			{
				throw Disconnected("I/O timeout");
			}
			pollfd pfd {.fd = nativeHandle(), .events = events, .revents = 0};
			const auto timeout = static_cast<int>(std::min<std::int64_t>(remaining.count() + 1, INT_MAX));
			const int result = ::poll(&pfd, 1, timeout);
			if (result < 0)
			{
				if (errno == EINTR)
				{
					continue;
				}
				platform::systemFail("poll");
			}
			if (result == 0)
			{
				throw Disconnected("I/O timeout");
			}
			if ((pfd.revents & POLLNVAL) != 0)
			{
				throw Disconnected("invalid transport descriptor");
			}
			if ((pfd.revents & (events | POLLERR | POLLHUP)) != 0)
			{
				return;
			}
		}
	}
} // namespace ftu::transfer::transport
