#include <ftu/transfer/protocol/channel.hpp>

#include <ftu/checksum/crc64.hpp>
#include <ftu/transfer/protocol/error.hpp>
#include <ftu/transfer/transport/disconnected.hpp>

#include <chrono>
#include <cstdint>
#include <poll.h>
#include <span>

namespace ftu::transfer::protocol
{
	Channel::Channel(std::unique_ptr<transport::Connection> connection) :
		m_connection(std::move(connection))
	{
		if (!m_connection)
		{
			throw std::invalid_argument("Channel requires a connection");
		}
	}

	std::size_t Channel::readIntoBuffer(std::span<std::uint8_t> destination, std::size_t bytes_read)
	{
		while (bytes_read < destination.size())
		{
			const auto result = m_connection->readSome(destination.subspan(bytes_read));
			if (result.status == transport::IoStatus::WouldBlock)
			{
				return bytes_read;
			}
			if (result.status == transport::IoStatus::End)
			{
				throw transport::Disconnected("peer closed the connection");
			}
			if (result.count == 0 || result.count > destination.size() - bytes_read)
			{
				throw std::logic_error("broken readSome contract");
			}
			bytes_read += result.count;
			m_last_activity = std::chrono::steady_clock::now();
		}
		return bytes_read;
	}

	std::optional<Message> Channel::receive()
	{
		m_header_bytes_read = readIntoBuffer(m_header_buffer, m_header_bytes_read);
		if (m_header_bytes_read < m_header_buffer.size())
		{
			return std::nullopt;
		}
		if (!m_parsed_header)
		{
			m_parsed_header = Message::decodeHeader(m_header_buffer);
			m_payload_buffer.resize(m_parsed_header->payload_size);
		}
		m_payload_bytes_read = readIntoBuffer(m_payload_buffer, m_payload_bytes_read);
		if (m_payload_bytes_read < m_payload_buffer.size())
		{
			return std::nullopt;
		}
		if (m_parsed_header->payload_crc64 != checksum::Crc64::compute(m_payload_buffer))
		{
			throw Error(ErrorCode::Integrity, "payload CRC64 mismatch");
		}
		Message frame {m_parsed_header->type, std::move(m_payload_buffer)};
		m_parsed_header.reset();
		m_header_bytes_read = 0;
		m_payload_bytes_read = 0;
		return frame;
	}

	void Channel::queue(Message frame)
	{
		if (pending())
		{
			throw std::logic_error("outgoing frame is still pending");
		}
		m_transmit_buffer = frame.encode();
		m_transmit_offset = 0;
	}

	bool Channel::flush()
	{
		while (m_transmit_offset < m_transmit_buffer.size())
		{
			const auto result = m_connection->writeSome(
				std::span<const std::uint8_t>(m_transmit_buffer).subspan(m_transmit_offset)
			);
			if (result.status == transport::IoStatus::WouldBlock)
			{
				return false;
			}
			if (result.status == transport::IoStatus::End)
			{
				throw transport::Disconnected("peer closed during write");
			}
			if (result.count == 0 || result.count > m_transmit_buffer.size() - m_transmit_offset)
			{
				throw std::logic_error("broken writeSome contract");
			}
			m_transmit_offset += result.count;
			m_last_activity = std::chrono::steady_clock::now();
		}
		m_transmit_buffer.clear();
		m_transmit_offset = 0;
		return true;
	}

	Message Channel::exchange(Message request, std::chrono::milliseconds timeout)
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		queue(std::move(request));
		while (!flush())
		{
			m_connection->waitReady(POLLOUT, deadline);
		}
		for (;;)
		{
			auto reply = receive();
			if (reply)
			{
				if (reply->type == MessageType::Error)
				{
					throw reply->asError();
				}
				return std::move(*reply);
			}
			m_connection->waitReady(POLLIN, deadline);
		}
	}

} // namespace ftu::transfer::protocol
