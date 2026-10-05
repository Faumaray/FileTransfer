#pragma once

#include "ftu/transfer/protocol/concepts.hpp"
#include "ftu/transfer/protocol/error.hpp"

#include <climits>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

namespace ftu::transfer::protocol
{

	class Decoder
	{
	public:
		template <BorrowedBuffer R>
		explicit Decoder(R&& input_bytes) :
			m_input_bytes(std::forward<R>(input_bytes))
		{
		}

		template <UnsignedInteger T>
		T readInteger()
		{
			if (sizeof(T) > remaining())
			{
				throw Error(ErrorCode::Protocol, "truncated payload");
			}
			std::uint64_t value = 0;
			for (std::size_t i = 0; i < sizeof(T); ++i)
			{
				value = (value << 8U) | m_input_bytes[m_read_offset++];
			}
			return static_cast<T>(value);
		}

		std::span<const std::uint8_t> readBytes(std::size_t count);
		std::string readString(std::size_t limit);

		std::size_t remaining() const noexcept { return m_input_bytes.size() - m_read_offset; }

		void requireEnd() const;

	private:
		std::span<const std::uint8_t> m_input_bytes;
		std::size_t m_read_offset = 0;
	};
} // namespace ftu::transfer::protocol
