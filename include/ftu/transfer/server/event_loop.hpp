#pragma once

#include <ftu/concurrency/thread_pool.hpp>
#include <ftu/platform/file_descriptor.hpp>
#include <ftu/transfer/options.hpp>
#include <ftu/transfer/server/client_handler.hpp>
#include <ftu/transfer/server/completion_queue.hpp>
#include <ftu/transfer/storage/session_store.hpp>
#include <ftu/transfer/transport/listener.hpp>

#include <chrono>
#include <csignal>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace ftu::transfer::server
{
	class EventLoop
	{
	public:
		EventLoop(
			std::unique_ptr<transport::Listener> listener,
			storage::SessionStore& store,
			Options options,
			sigset_t signal_mask
		);
		~EventLoop();
		EventLoop(const EventLoop&) = delete;
		EventLoop& operator=(const EventLoop&) = delete;
		void run();

	private:
		static constexpr std::uint64_t M_LISTENER_TOKEN = 1;
		static constexpr std::uint64_t M_WAKE_TOKEN = 2;
		static constexpr std::uint64_t M_SIGNAL_TOKEN = 3;
		bool handle(std::uint64_t token);
		void drainSignals();
		void control(int operation, int fd, std::uint32_t flags, std::uint64_t token);
		void erase(std::uint64_t token);
		void acceptReady();
		void dispatch(std::uint64_t token);
		void completeReady();
		void maintenance();
		int waitTimeout() const;
		std::unique_ptr<transport::Listener> m_listener;
		storage::SessionStore& m_store;
		Options m_options;
		platform::FileDescriptor m_epoll;
		CompletionQueue m_completions;
		platform::FileDescriptor m_signals;
		std::unordered_map<std::uint64_t, std::shared_ptr<ClientHandler>> m_clients;
		std::uint64_t m_next_token = 4;
		bool m_accept_paused = false;
		std::chrono::steady_clock::time_point m_accept_again;
		concurrency::ThreadPool m_pool;
	};
} // namespace ftu::transfer::server
