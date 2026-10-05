#include <ftu/transfer/protocol/decoder.hpp>

#include <cstdint>
#include <span>

namespace ftu::transfer::protocol
{
	std::span<const std::uint8_t> Decoder::readBytes(std::size_t count)
	{
		if (count > remaining())
		{
			throw Error(ErrorCode::Protocol, "truncated payload");
		}
		auto result = m_input_bytes.subspan(m_read_offset, count);
		m_read_offset += count;
		return result;
	}

	std::string Decoder::readString(std::size_t limit)
	{
		const auto length = readInteger<std::uint32_t>();
		if (length > limit)
		{
			throw Error(ErrorCode::Limit, "string limit exceeded");
		}
		const auto value = readBytes(length);
		return std::string(value.begin(), value.end());
	}

	void Decoder::requireEnd() const
	{
		if (remaining() != 0)
		{
			throw Error(ErrorCode::Protocol, "unexpected trailing payload");
		}
	}
} // namespace ftu::transfer::protocol
