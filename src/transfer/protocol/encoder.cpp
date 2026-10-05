#include "ftu/transfer/protocol/encoder.hpp"
#include "ftu/transfer/protocol/error.hpp"

#include <cstdint>
#include <limits>
#include <span>

namespace ftu::transfer::protocol
{
	void Encoder::writeBytes(std::span<const std::uint8_t> source)
	{
		m_encoded_bytes.insert(m_encoded_bytes.end(), source.begin(), source.end());
	}

	void Encoder::writeTextBytes(std::string_view text)
	{
		for (const char byte : text)
		{
			m_encoded_bytes.push_back(static_cast<std::uint8_t>(byte));
		}
	}

	void Encoder::writeString(std::string_view text)
	{
		if (text.size() > std::numeric_limits<std::uint32_t>::max())
		{
			throw Error(ErrorCode::Limit, "string too long");
		}
		writeInteger<std::uint32_t>(static_cast<std::uint32_t>(text.size()));
		writeTextBytes(text);
	}
} // namespace ftu::transfer::protocol
