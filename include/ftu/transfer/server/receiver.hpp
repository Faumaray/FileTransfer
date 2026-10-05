#pragma once

#include "ftu/transfer/protocol/message.hpp"
#include "ftu/transfer/storage/session.hpp"
#include "ftu/transfer/storage/session_store.hpp"

#include <memory>

namespace ftu::transfer::server
{
	class Receiver
	{
	public:
		explicit Receiver(storage::SessionStore& store);
		~Receiver();
		Receiver(const Receiver&) = delete;
		Receiver& operator=(const Receiver&) = delete;
		protocol::Message handle(protocol::Message frame);

		[[nodiscard]] bool terminal() const noexcept { return m_terminal; }

	private:
		protocol::Message begin(const protocol::Message& frame);
		protocol::Message append(const protocol::Message& frame);
		protocol::Message restart(const protocol::Message& frame);
		protocol::Message finish(const protocol::Message& frame);

		storage::SessionStore& m_store;
		std::unique_ptr<storage::Session> m_session;
		bool m_terminal = false;
		bool m_data_seen = false;
	};
} // namespace ftu::transfer::server
