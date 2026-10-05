#include <ftu/transfer/server/receiver.hpp>

#include <ftu/logging/logger.hpp>
#include <ftu/transfer/protocol/decoder.hpp>
#include <ftu/transfer/protocol/error.hpp>
#include <ftu/transfer/protocol/file_metadata.hpp>

#include <cstdint>

namespace ftu::transfer::server
{
	Receiver::Receiver(storage::SessionStore& store) :
		m_store(store)
	{
	}

	Receiver::~Receiver()
	{
		if (m_session && !m_session->completed())
		{
			logging::Logger::warn(
				"SERVER INTERRUPTED uuid={} received={}/{} (partial retained)",
				m_session->metadata().uuid,
				m_session->offset(),
				m_session->metadata().size
			);
		}
	}

	protocol::Message Receiver::handle(protocol::Message frame)
	{
		if (m_terminal)
		{
			throw protocol::Error(protocol::ErrorCode::Protocol, "session already finished");
		}
		if (!m_session)
		{
			return begin(frame);
		}
		switch (frame.type)
		{
			case protocol::MessageType::Data:
				return append(frame);
			case protocol::MessageType::Reset:
				return restart(frame);
			case protocol::MessageType::Finish:
				return finish(frame);
			default:
				throw protocol::Error(protocol::ErrorCode::Protocol, "unexpected frame in receiving state");
		}
	}

	protocol::Message Receiver::begin(const protocol::Message& frame)
	{
		if (frame.type != protocol::MessageType::Hello)
		{
			throw protocol::Error(protocol::ErrorCode::Protocol, "first frame must be HELLO");
		}
		m_session = m_store.acquire(protocol::FileMetadata::decode(frame.payload));
		const auto& metadata = m_session->metadata();
		logging::Logger::info(
			"SERVER BEGIN uuid={} name=\"{}\" size={} offset={}",
			metadata.uuid,
			metadata.name,
			metadata.size,
			m_session->offset()
		);
		if (m_session->completed())
		{
			m_terminal = true;
			logging::Logger::info(
				"SERVER COMPLETE uuid={} saved={} already-present", metadata.uuid, m_session->outputName()
			);
			return protocol::Message::done(m_session->outputName());
		}
		return protocol::Message::progress(
			protocol::MessageType::Ready, m_session->offset(), m_session->crc()
		);
	}

	protocol::Message Receiver::append(const protocol::Message& frame)
	{
		protocol::Decoder in(frame.payload);
		const auto offset = in.readInteger<std::uint64_t>();
		m_session->append(offset, in.readBytes(in.remaining()));
		m_data_seen = true;
		return protocol::Message::progress(protocol::MessageType::Ack, m_session->offset(), m_session->crc());
	}

	protocol::Message Receiver::restart(const protocol::Message& frame)
	{
		if (!frame.payload.empty() || m_data_seen)
		{
			throw protocol::Error(protocol::ErrorCode::Protocol, "RESET allowed only before DATA");
		}
		m_session->reset();
		logging::Logger::warn("SERVER RESET uuid={}", m_session->metadata().uuid);
		return protocol::Message::progress(protocol::MessageType::Ready, 0, 0);
	}

	protocol::Message Receiver::finish(const protocol::Message& frame)
	{
		if (!frame.payload.empty())
		{
			throw protocol::Error(protocol::ErrorCode::Protocol, "FINISH must have an empty payload");
		}
		m_session->finish();
		m_terminal = true;
		logging::Logger::info(
			"SERVER COMPLETE uuid={} saved={} crc64={:x}",
			m_session->metadata().uuid,
			m_session->outputName(),
			m_session->crc()
		);
		return protocol::Message::done(m_session->outputName());
	}
} // namespace ftu::transfer::server
