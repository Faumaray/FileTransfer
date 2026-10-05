#pragma once

#include "ftu/transfer/protocol/constants.hpp"
#include "ftu/transfer/protocol/header.hpp"
#include "ftu/transfer/protocol/message.hpp"
#include "ftu/transfer/transport/connection.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace ftu::transfer::protocol
{
	class Channel
	{
	public:
		explicit Channel(std::unique_ptr<transport::Connection> connection);

		[[nodiscard]] int nativeHandle() const noexcept { return m_connection->nativeHandle(); }

		[[nodiscard]] std::chrono::steady_clock::time_point lastActivity() const noexcept
		{
			return m_last_activity;
		}

		std::optional<Message> receive();
		void queue(Message frame);
		bool flush();

		bool pending() const noexcept { return !m_transmit_buffer.empty(); }

		Message exchange(Message request, std::chrono::milliseconds timeout);

	private:
		std::size_t readIntoBuffer(std::span<std::uint8_t> destination, std::size_t bytes_read);
		std::unique_ptr<transport::Connection> m_connection;
		std::vector<std::uint8_t> m_header_buffer = std::vector<std::uint8_t>(MESSAGE_HEADER_SIZE);
		std::size_t m_header_bytes_read = 0;
		std::optional<Header> m_parsed_header;
		std::vector<std::uint8_t> m_payload_buffer;
		std::size_t m_payload_bytes_read = 0;
		std::vector<std::uint8_t> m_transmit_buffer;
		std::size_t m_transmit_offset = 0;
		std::chrono::steady_clock::time_point m_last_activity = std::chrono::steady_clock::now();
	};
} // namespace ftu::transfer::protocol
