#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace ftu::concurrency
{
	class ThreadPool
	{
		// TODO: upgrade minimal gcc version and use jthread
	public:
		explicit ThreadPool(std::size_t worker_count, std::size_t capacity);
		~ThreadPool();
		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;
		bool submit(std::function<void()> task);
		void shutdown() noexcept;

	private:
		void worker();
		std::mutex m_mutex;
		std::condition_variable m_ready;
		std::deque<std::function<void()>> m_queue;
		std::vector<std::thread> m_threads;
		std::size_t m_capacity;
		bool m_stopping = false;
	};

} // namespace ftu::concurrency
