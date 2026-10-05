#include <ftu/transfer/server.hpp>

#include <ftu/transfer/server/event_loop.hpp>
#include <ftu/transfer/server/signal_guard.hpp>

#include <stdexcept>
#include <utility>

namespace ftu::transfer
{
	Server::Server(
		std::unique_ptr<transport::Listener> listener, storage::SessionStore& store, Options options
	) :
		m_listener(std::move(listener)),
		m_store(store),
		m_options(std::move(options))
	{
		if (!m_listener)
		{
			throw std::invalid_argument("Server requires a listener");
		}
	}

	void Server::run()
	{
		if (!m_listener)
		{
			throw std::logic_error("Server::run can only be called once");
		}
		server::SignalGuard signals;
		server::EventLoop eventLoop(std::move(m_listener), m_store, m_options, signals.mask());
		eventLoop.run();
	}
} // namespace ftu::transfer
