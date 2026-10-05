#include <ftu/transfer/server/event_loop.hpp>

#include <ftu/logging/logger.hpp>
#include <ftu/platform/system_error.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdint>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <unistd.h>

namespace ftu::transfer::server
{
	EventLoop::EventLoop(
		std::unique_ptr<transport::Listener> listener,
		storage::SessionStore& store,
		Options options,
		sigset_t signal_mask
	) :
		m_listener(std::move(listener)),
		m_store(store),
		m_options(std::move(options)),
		m_epoll(platform::check(::epoll_create1(EPOLL_CLOEXEC), "epoll_create1")),
		m_signals(platform::check(::signalfd(-1, &signal_mask, SFD_NONBLOCK | SFD_CLOEXEC), "signalfd")),
		m_pool(m_options.worker_count, m_options.max_connections)
	{
		if (!m_listener)
		{
			throw std::invalid_argument("EventLoop requires a listener");
		}
		control(EPOLL_CTL_ADD, m_listener->nativeHandle(), EPOLLIN, M_LISTENER_TOKEN);
		control(EPOLL_CTL_ADD, m_completions.nativeHandle(), EPOLLIN, M_WAKE_TOKEN);
		control(EPOLL_CTL_ADD, m_signals.get(), EPOLLIN, M_SIGNAL_TOKEN);
	}

	EventLoop::~EventLoop()
	{
		m_pool.shutdown();
		m_clients.clear();
	}

	void EventLoop::run()
	{
		logging::Logger::info(
			"SERVER LISTEN socket={} output=\"{}\" workers={}",
			m_options.socket_path,
			m_options.output_directory.native(),
			m_options.worker_count
		);
		std::array<epoll_event, 64> events {};
		for (bool running = true; running;)
		{
			maintenance();
			const int count =
				::epoll_wait(m_epoll.get(), events.data(), static_cast<int>(events.size()), waitTimeout());
			if (count < 0 && errno != EINTR)
			{
				platform::systemFail("epoll_wait");
			}
			for (int i = 0; i < count && running; ++i)
			{
				running = handle(events[static_cast<std::size_t>(i)].data.u64);
			}
		}
		logging::Logger::info("SERVER STOP (unfinished sessions are retained)");
	}

	bool EventLoop::handle(std::uint64_t token)
	{
		switch (token)
		{
			case M_SIGNAL_TOKEN:
				drainSignals();
				return false;
			case M_LISTENER_TOKEN:
				acceptReady();
				return true;
			case M_WAKE_TOKEN:
				completeReady();
				return true;
			default:
				dispatch(token);
				return true;
		}
	}

	void EventLoop::drainSignals()
	{
		signalfd_siginfo info {};
		for (;;)
		{
			const auto count = ::read(m_signals.get(), &info, sizeof(info));
			if (count > 0 || (count < 0 && errno == EINTR))
			{
				continue;
			}
			return;
		}
	}

	void EventLoop::control(int operation, int fd, std::uint32_t flags, std::uint64_t token)
	{
		epoll_event event {};
		event.events = flags;
		event.data.u64 = token;
		platform::check(::epoll_ctl(m_epoll.get(), operation, fd, &event), "epoll_ctl");
	}

	void EventLoop::erase(std::uint64_t token)
	{
		const auto it = m_clients.find(token);
		if (it == m_clients.end())
		{
			return;
		}
		::epoll_ctl(m_epoll.get(), EPOLL_CTL_DEL, it->second->nativeHandle(), nullptr);
		m_clients.erase(it);
	}

	void EventLoop::acceptReady()
	{
		if (m_accept_paused)
		{
			return;
		}
		try
		{
			for (unsigned n = 0; n < 32; ++n)
			{
				auto connection = m_listener->acceptOne();
				if (!connection)
				{
					return;
				}
				if (m_clients.size() >= m_options.max_connections)
				{
					continue;
				}
				const auto token = m_next_token++;
				auto client = std::make_shared<ClientHandler>(std::move(connection), m_store);
				control(EPOLL_CTL_ADD, client->nativeHandle(), EPOLLIN | EPOLLONESHOT, token);
				m_clients.emplace(token, std::move(client));
			}
		}
		catch (const std::exception& error)
		{
			logging::Logger::warn("SERVER ACCEPT PAUSED \"{}\"", error.what());
			::epoll_ctl(m_epoll.get(), EPOLL_CTL_DEL, m_listener->nativeHandle(), nullptr);
			m_accept_paused = true;
			m_accept_again = std::chrono::steady_clock::now() + std::chrono::seconds(1);
		}
	}

	void EventLoop::dispatch(std::uint64_t token)
	{
		const auto it = m_clients.find(token);
		if (it == m_clients.end() || it->second->busy())
		{
			return;
		}
		const auto client = it->second;
		client->setBusy(true);
		const bool accepted = m_pool.submit(
			[token, client, &completions = m_completions]
			{
				completions.push({.connection_token = token, .ready_events = client->pump()});
			}
		);
		if (!accepted)
		{
			erase(token);
		}
	}

	void EventLoop::completeReady()
	{
		const auto completed = m_completions.drain();
		for (const auto& result : completed)
		{
			const auto it = m_clients.find(result.connection_token);
			if (it == m_clients.end())
			{
				continue;
			}
			const auto client = it->second;
			client->setBusy(false);
			if (result.ready_events == 0)
			{
				erase(result.connection_token);
			}
			else
			{
				control(
					EPOLL_CTL_MOD,
					client->nativeHandle(),
					result.ready_events | EPOLLONESHOT,
					result.connection_token
				);
			}
		}
	}

	void EventLoop::maintenance()
	{
		const auto now = std::chrono::steady_clock::now();
		if (m_accept_paused && now >= m_accept_again)
		{
			control(EPOLL_CTL_ADD, m_listener->nativeHandle(), EPOLLIN, M_LISTENER_TOKEN);
			m_accept_paused = false;
		}
		for (auto it = m_clients.begin(); it != m_clients.end();)
		{
			const auto token = it->first;
			const auto client = it->second;
			const bool expired =
				!client->busy() && now - client->lastActivity() >= m_options.inactivity_timeout;
			++it;
			if (expired)
			{
				logging::Logger::warn("SERVER TIMEOUT connection={}", token);
				erase(token);
			}
		}
	}

	int EventLoop::waitTimeout() const
	{
		auto nearest = std::chrono::steady_clock::time_point::max();
		if (m_accept_paused)
		{
			nearest = m_accept_again;
		}
		for (const auto& entry : m_clients)
		{
			if (!entry.second->busy())
			{
				nearest = std::min(nearest, entry.second->lastActivity() + m_options.inactivity_timeout);
			}
		}
		if (nearest == std::chrono::steady_clock::time_point::max())
		{
			return -1;
		}
		const auto ms =
			std::chrono::duration_cast<std::chrono::milliseconds>(nearest - std::chrono::steady_clock::now())
				.count();
		return static_cast<int>(std::clamp<std::int64_t>(ms + 1, 0, INT_MAX));
	}
} // namespace ftu::transfer::server
