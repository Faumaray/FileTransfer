#pragma once

#include <ftu/transfer/options.hpp>
#include <ftu/transfer/storage/session_store.hpp>
#include <ftu/transfer/transport/listener.hpp>

#include <memory>

namespace ftu::transfer
{
	class Server
	{
	public:
		Server(std::unique_ptr<transport::Listener> listener, storage::SessionStore& store, Options options);
		Server(const Server&) = delete;
		Server& operator=(const Server&) = delete;
		void run();

	private:
		std::unique_ptr<transport::Listener> m_listener;
		storage::SessionStore& m_store;
		Options m_options;
	};
} // namespace ftu::transfer
