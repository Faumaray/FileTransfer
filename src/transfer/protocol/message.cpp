#include <ftu/transfer/protocol/message.hpp>

#include <ftu/checksum/crc64.hpp>
#include <ftu/transfer/protocol/constants.hpp>
#include <ftu/transfer/protocol/decoder.hpp>
#include <ftu/transfer/protocol/encoder.hpp>
#include <ftu/transfer/protocol/file_metadata.hpp>
namespace ftu::transfer::protocol
{
	Message Message::progress(MessageType type, std::uint64_t offset, std::uint64_t crc)
	{
		Encoder out;
		out.writeInteger<std::uint64_t>(offset);
		out.writeInteger<std::uint64_t>(crc);
		return {type, std::move(out).releaseBuffer()};
	}

	Message Message::error(ErrorCode code, std::string_view text)
	{
		Encoder out;
		out.writeInteger<std::uint32_t>(static_cast<std::uint32_t>(code));
		out.writeString(text.substr(0, 1024));
		return {MessageType::Error, std::move(out).releaseBuffer()};
	}

	Message Message::done(std::string_view saved_name)
	{
		Encoder out;
		out.writeString(saved_name);
		return {MessageType::Done, std::move(out).releaseBuffer()};
	}

	Progress Message::asProgress(MessageType expected) const
	{
		if (type != expected)
		{
			throw Error(ErrorCode::Protocol, "unexpected reply type");
		}
		Decoder in(payload);
		const auto offset = in.readInteger<std::uint64_t>();
		const auto crc = in.readInteger<std::uint64_t>();
		in.requireEnd();
		return {offset, crc};
	}

	Error Message::asError() const
	{
		if (type != MessageType::Error)
		{
			throw Error(ErrorCode::Protocol, "unexpected reply type");
		}
		Decoder in(payload);
		const auto code = in.readInteger<std::uint32_t>();
		const auto text = in.readString(1024);
		in.requireEnd();
		if (code < 1 || code > 6)
		{
			throw Error(ErrorCode::Protocol, "unknown remote error code");
		}
		return {static_cast<ErrorCode>(code), "server: " + text};
	}

	std::string Message::asSavedName() const
	{
		if (type != MessageType::Done)
		{
			throw Error(ErrorCode::Protocol, "expected final DONE acknowledgement");
		}
		Decoder in(payload);
		auto name = in.readString(64);
		in.requireEnd();
		FileMetadata::validateBasename(name);
		return name;
	}

	std::vector<std::uint8_t> Message::encode() const
	{
		if (payload.size() > MAX_PAYLOAD_SIZE)
		{
			throw Error(ErrorCode::Limit, "frame too large");
		}
		Encoder out;
		out.writeTextBytes("FTU1");
		out.writeInteger<std::uint16_t>(1);
		out.writeInteger<std::uint16_t>(static_cast<std::uint16_t>(type));
		out.writeInteger<std::uint32_t>(static_cast<std::uint32_t>(payload.size()));
		out.writeInteger<std::uint32_t>(0); // Reserved; covered by header CRC.
		out.writeInteger<std::uint64_t>(checksum::Crc64::compute(payload));
		out.writeInteger<std::uint64_t>(checksum::Crc64::compute(out.encodedData()));
		out.writeBytes(payload);
		return std::move(out).releaseBuffer();
	}

	Header Message::decodeHeader(std::span<const std::uint8_t> bytes)
	{
		if (bytes.size() != MESSAGE_HEADER_SIZE)
		{
			throw Error(ErrorCode::Protocol, "invalid header size");
		}
		Decoder in(bytes);
		const auto magic = in.readBytes(4);
		if (std::string(magic.begin(), magic.end()) != "FTU1")
		{
			throw Error(ErrorCode::Protocol, "invalid frame magic");
		}
		const auto version = in.readInteger<std::uint16_t>();
		const auto rawType = in.readInteger<std::uint16_t>();
		const auto size = in.readInteger<std::uint32_t>();
		const auto reserved = in.readInteger<std::uint32_t>();
		const auto payloadCrc = in.readInteger<std::uint64_t>();
		if (in.readInteger<std::uint64_t>() != checksum::Crc64::compute(bytes.first(24)))
		{
			throw Error(ErrorCode::Integrity, "header CRC64 mismatch");
		}
		if (version != 1 || rawType < 1 || rawType > 8 || reserved != 0)
		{
			throw Error(ErrorCode::Protocol, "unsupported frame version/type/flags");
		}
		if (size > MAX_PAYLOAD_SIZE)
		{
			throw Error(ErrorCode::Limit, "payload limit exceeded");
		}
		return {static_cast<MessageType>(rawType), size, payloadCrc};
	}

	Message Message::decode(std::span<const std::uint8_t> bytes)
	{
		if (bytes.size() < MESSAGE_HEADER_SIZE)
		{
			throw Error(ErrorCode::Protocol, "truncated header");
		}
		const Header header = decodeHeader(bytes.first(MESSAGE_HEADER_SIZE));
		if (bytes.size() != MESSAGE_HEADER_SIZE + header.payload_size)
		{
			throw Error(ErrorCode::Protocol, "invalid frame size");
		}
		std::vector<std::uint8_t> body(bytes.begin() + MESSAGE_HEADER_SIZE, bytes.end());
		if (header.payload_crc64 != checksum::Crc64::compute(body))
		{
			throw Error(ErrorCode::Integrity, "payload CRC64 mismatch");
		}
		return {header.type, std::move(body)};
	}
} // namespace ftu::transfer::protocol
