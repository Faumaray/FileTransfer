#include "ftu/transfer/client.hpp"
#include "ftu/checksum/crc64.hpp"
#include "ftu/logging/logger.hpp"
#include "ftu/transfer/protocol/constants.hpp"
#include "ftu/transfer/protocol/encoder.hpp"
#include "ftu/transfer/protocol/error.hpp"
#include "ftu/transfer/protocol/session_id.hpp"
#include "ftu/transfer/transport/disconnected.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <span>
#include <stdexcept>
#include <system_error>
#include <thread>

namespace ftu::transfer
{
	namespace
	{
		bool transient(const std::exception& error)
		{
			if (const auto* fault = dynamic_cast<const protocol::Error*>(&error))
			{
				return fault->code() == protocol::ErrorCode::Busy ||
					   fault->code() == protocol::ErrorCode::Integrity;
			}
			if (const auto* failure = dynamic_cast<const std::system_error*>(&error))
			{
				const int code = failure->code().value();
				return code == ECONNRESET || code == ECONNREFUSED || code == EPIPE || code == ENOENT ||
					   code == EAGAIN || code == ETIMEDOUT || code == ENOTCONN || code == EINTR;
			}
			return dynamic_cast<const transport::Disconnected*>(&error) != nullptr;
		}
	} // namespace

	Client::Client(
		std::unique_ptr<transport::Connector> connector, Options options, std::filesystem::path path
	) :
		m_connector(std::move(connector)),
		m_options(std::move(options)),
		m_source(std::move(path))
	{
		if (!m_connector)
		{
			throw std::invalid_argument("Client requires a connector");
		}
		m_metadata.name = m_source.path().filename().string();
		protocol::FileMetadata::validateBasename(m_metadata.name);
		m_metadata.size = m_source.size();
		if (m_options.max_file_size != 0 && (m_metadata.size > m_options.max_file_size))
		{
			throw std::runtime_error("source exceeds FTU_MAX_FILE_SIZE");
		}
	}

	void Client::run()
	{
		logging::Logger::trace("CLIENT HASH name=\"{}\" size={}", m_metadata.name, m_metadata.size);
		m_metadata.crc = m_source.crc64(m_metadata.size);
		m_source.checkUnchanged();
		m_metadata.uuid = protocol::SessionId::of(m_metadata.crc, m_metadata.name, m_metadata.size);
		logging::Logger::trace(
			"CLIENT BEGIN uuid={} size={} crc64={:x}", m_metadata.uuid, m_metadata.size, m_metadata.crc
		);
		for (unsigned trial = 0;; ++trial)
		{
			try
			{
				const auto name = attempt();
				logging::Logger::trace(
					"CLIENT COMPLETE uuid={} saved={} bytes={}", m_metadata.uuid, name, m_metadata.size
				);
				return;
			}
			catch (const std::exception& error)
			{
				if (trial >= m_options.retry_count || !transient(error))
				{
					throw;
				}
				logging::Logger::warn(
					"CLIENT RETRY {}/{} reason=\"{}\"", trial + 1, m_options.retry_count, error.what()
				);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(250U << std::min(trial, 3U)));
		}
	}

	std::string Client::attempt()
	{
		m_source.checkUnchanged();
		protocol::Channel channel(m_connector->connect(m_options.inactivity_timeout));
		auto reply = channel.exchange(
			{.type = protocol::MessageType::Hello, .payload = m_metadata.encode()},
			m_options.inactivity_timeout
		);
		if (reply.type == protocol::MessageType::Done)
		{
			m_source.checkUnchanged();
			return reply.asSavedName();
		}
		const auto crc = upload(channel, resume(channel, reply));
		m_source.checkUnchanged();
		if (crc != m_metadata.crc)
		{
			throw std::runtime_error("source CRC changed during transfer");
		}
		auto name =
			channel
				.exchange(
					{.type = protocol::MessageType::Finish, .payload = {}}, m_options.inactivity_timeout
				)
				.asSavedName();
		m_source.checkUnchanged();
		return name;
	}

	protocol::Progress Client::resume(protocol::Channel& channel, const protocol::Message& ready)
	{
		auto progress = ready.asProgress(protocol::MessageType::Ready);
		if (progress.byte_offset > m_metadata.size)
		{
			throw protocol::Error(protocol::ErrorCode::Protocol, "resume offset exceeds source size");
		}
		const auto prefixCrc = m_source.crc64(progress.byte_offset);
		m_source.checkUnchanged();
		if (prefixCrc == progress.prefix_crc64)
		{
			return progress;
		}
		logging::Logger::warn("CLIENT PREFIX MISMATCH: restarting the saved partial");
		progress =
			channel
				.exchange({.type = protocol::MessageType::Reset, .payload = {}}, m_options.inactivity_timeout)
				.asProgress(protocol::MessageType::Ready);
		if (progress.byte_offset != 0 || progress.prefix_crc64 != 0)
		{
			throw protocol::Error(protocol::ErrorCode::Protocol, "invalid RESET acknowledgement");
		}
		return progress;
	}

	std::uint64_t Client::upload(protocol::Channel& channel, protocol::Progress start)
	{
		logging::Logger::trace("CLIENT RESUME uuid={} offset={}", m_metadata.uuid, start.byte_offset);
		auto offset = start.byte_offset;
		checksum::Crc64 running(start.prefix_crc64);
		std::array<std::uint8_t, protocol::FILE_CHUNK_SIZE> buffer {};
		while (offset < m_metadata.size)
		{
			const auto chunk = std::span<std::uint8_t>(buffer).first(
				static_cast<std::size_t>(std::min<std::uint64_t>(buffer.size(), m_metadata.size - offset))
			);
			m_source.read(chunk, offset);
			running.update(chunk);
			protocol::Encoder payload;
			payload.writeInteger<std::uint64_t>(offset);
			payload.writeBytes(chunk);
			const auto ack =
				channel
					.exchange(
						{.type = protocol::MessageType::Data, .payload = std::move(payload).releaseBuffer()},
						m_options.inactivity_timeout
					)
					.asProgress(protocol::MessageType::Ack);
			if (ack.byte_offset != offset + chunk.size() || ack.prefix_crc64 != running.value())
			{
				throw protocol::Error(
					protocol::ErrorCode::Integrity, "ACK offset/CRC64 does not match sent data"
				);
			}
			offset += chunk.size();
		}
		return running.value();
	}
} // namespace ftu::transfer
