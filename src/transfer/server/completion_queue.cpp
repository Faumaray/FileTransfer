#include "ftu/transfer/server/completion_queue.hpp"
#include "ftu/platform/system_error.hpp"

#include <cerrno>
#include <cstdint>
#include <sys/eventfd.h>
#include <unistd.h>

namespace ftu::transfer::server
{
	CompletionQueue::CompletionQueue() :
		m_wake(platform::check(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC), "eventfd"))
	{
	}

	void CompletionQueue::push(Completion completion)
	{
		{
			std::lock_guard lock(m_mutex);
			m_completed.push_back(completion);
		}
		const std::uint64_t one = 1;
		while (::write(m_wake.get(), &one, sizeof(one)) < 0 && errno == EINTR)
		{
		}
	}

	std::deque<Completion> CompletionQueue::drain()
	{
		std::uint64_t value = 0;
		while (::read(m_wake.get(), &value, sizeof(value)) < 0 && errno == EINTR)
		{
		}
		std::deque<Completion> result;
		{
			std::lock_guard lock(m_mutex);
			result.swap(m_completed);
		}
		return result;
	}
} // namespace ftu::transfer::server
