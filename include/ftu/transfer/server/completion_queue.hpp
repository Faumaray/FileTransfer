#pragma once

#include "ftu/platform/file_descriptor.hpp"
#include "ftu/transfer/server/completion.hpp"

#include <cstdint>
#include <deque>
#include <mutex>

namespace ftu::transfer::server
{
	class CompletionQueue
	{
	public:
		CompletionQueue();

		[[nodiscard]] int nativeHandle() const noexcept { return m_wake.get(); }

		void push(Completion completion);
		std::deque<Completion> drain();

	private:
		platform::FileDescriptor m_wake;
		std::mutex m_mutex;
		std::deque<Completion> m_completed;
	};
} // namespace ftu::transfer::server
