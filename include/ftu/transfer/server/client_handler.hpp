#pragma once

#include "ftu/transfer/protocol/channel.hpp"
#include "ftu/transfer/protocol/error_code.hpp"
#include "ftu/transfer/server/receiver.hpp"
#include "ftu/transfer/storage/session_store.hpp"
#include "ftu/transfer/transport/connection.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>

namespace ftu::transfer::server
{
	class ClientHandler
	{
	public:
		ClientHandler(std::unique_ptr<transport::Connection> connection, storage::SessionStore& store);

		[[nodiscard]] int nativeHandle() const noexcept { return m_channel.nativeHandle(); }

		[[nodiscard]] std::chrono::steady_clock::time_point lastActivity() const noexcept
		{
			return m_channel.lastActivity();
		}

		[[nodiscard]] bool busy() const noexcept { return m_busy; }

		void setBusy(bool value) noexcept { m_busy = value; }

		std::uint32_t pump() noexcept;

	private:
		std::uint32_t fail(protocol::ErrorCode code, std::string_view message) noexcept;
		protocol::Channel m_channel;
		Receiver m_receiver;
		bool m_busy = false;
		bool m_closing = false;
	};
} // namespace ftu::transfer::server
