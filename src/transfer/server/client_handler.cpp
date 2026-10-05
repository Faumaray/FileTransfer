#include <ftu/transfer/server/client_handler.hpp>

#include <ftu/logging/logger.hpp>
#include <ftu/transfer/protocol/error.hpp>
#include <ftu/transfer/protocol/message.hpp>
#include <ftu/transfer/transport/disconnected.hpp>

#include <cerrno>
#include <cstdint>
#include <sys/epoll.h>
#include <system_error>

namespace ftu::transfer::server
{
	ClientHandler::ClientHandler(
		std::unique_ptr<transport::Connection> connection, storage::SessionStore& store
	) :
		m_channel(std::move(connection)),
		m_receiver(store)
	{
	}

	std::uint32_t ClientHandler::pump() noexcept
	{
		try
		{
			for (unsigned step = 0; step < 16; ++step)
			{
				if (m_channel.pending())
				{
					if (!m_channel.flush())
					{
						return EPOLLOUT;
					}
					if (m_closing)
					{
						return 0;
					}
				}
				auto frame = m_channel.receive();
				if (!frame)
				{
					return EPOLLIN;
				}
				m_channel.queue(m_receiver.handle(std::move(*frame)));
				m_closing = m_receiver.terminal();
			}
			return m_channel.pending() ? EPOLLOUT : EPOLLIN;
		}
		catch (const protocol::Error& fault)
		{
			return fail(fault.code(), fault.what());
		}
		catch (const transport::Disconnected& error)
		{
			logging::Logger::warn("SERVER DISCONNECT \"{}\"", error.what());
			return 0;
		}
		catch (const std::system_error& error)
		{
			if (error.code().value() == EPIPE || error.code().value() == ECONNRESET)
			{
				return 0;
			}
			return fail(protocol::ErrorCode::Storage, error.what());
		}
		catch (const std::exception& error)
		{
			return fail(protocol::ErrorCode::Storage, error.what());
		}
	}

	std::uint32_t ClientHandler::fail(protocol::ErrorCode code, std::string_view message) noexcept
	{
		try
		{
			logging::Logger::error("SERVER ERROR \"{}\"", message);
			if (m_channel.pending())
			{
				return 0;
			}
			m_channel.queue(protocol::Message::error(code, message));
			m_closing = true;
			return m_channel.flush() ? 0U : static_cast<std::uint32_t>(EPOLLOUT);
		}
		catch (...)
		{
			return 0;
		}
	}
} // namespace ftu::transfer::server
