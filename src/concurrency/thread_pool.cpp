#include "ftu/concurrency/thread_pool.hpp"

#include <stdexcept>

namespace ftu::concurrency
{
	ThreadPool::ThreadPool(std::size_t worker_count, std::size_t capacity) :
		m_capacity(capacity)
	{
		if (worker_count == 0 || capacity == 0)
		{
			throw std::invalid_argument("empty thread pool");
		}
		try
		{
			for (std::size_t i = 0; i < worker_count; ++i)
			{
				m_threads.emplace_back(
					[this]
					{
						worker();
					}
				);
			}
		}
		catch (...)
		{
			shutdown();
			throw;
		}
	}

	ThreadPool::~ThreadPool()
	{
		shutdown();
	}

	bool ThreadPool::submit(std::function<void()> task)
	{
		if (!task)
		{
			throw std::invalid_argument("empty thread-pool task");
		}
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_stopping || m_queue.size() >= m_capacity)
			{
				return false;
			}
			m_queue.push_back(std::move(task));
		}
		m_ready.notify_one();
		return true;
	}

	void ThreadPool::shutdown() noexcept
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_stopping = true;
		}
		m_ready.notify_all();
		for (auto& thread : m_threads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
		m_threads.clear();
	}

	void ThreadPool::worker()
	{
		for (;;)
		{
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_ready.wait(
					lock,
					[this]
					{
						return m_stopping || !m_queue.empty();
					}
				);
				if (m_queue.empty())
				{
					return;
				}
				task = std::move(m_queue.front());
				m_queue.pop_front();
			}
			task();
		}
	}
} // namespace ftu::concurrency
