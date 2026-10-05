#pragma once

#include <climits>
#include <cstdint>
#include <ftu/transfer/protocol/concepts.hpp>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace ftu::transfer::protocol
{

	class Encoder
	{
	public:
		template <UnsignedInteger T>
		void writeInteger(T value)
		{
			for (std::size_t remaining = sizeof(T); remaining != 0; --remaining)
			{
				const auto shift = static_cast<unsigned>((remaining - 1) * 8);
				m_encoded_bytes.push_back(static_cast<std::uint8_t>(value >> shift));
			}
		}

		void writeBytes(std::span<const std::uint8_t> source);
		void writeTextBytes(std::string_view text);
		void writeString(std::string_view text);

		[[nodiscard]] std::span<const std::uint8_t> encodedData() const& noexcept { return m_encoded_bytes; }

		[[nodiscard]] std::span<const std::uint8_t> encodedData() const&& = delete;

		std::vector<std::uint8_t> releaseBuffer() && noexcept { return std::move(m_encoded_bytes); }

	private:
		std::vector<std::uint8_t> m_encoded_bytes;
	};
} // namespace ftu::transfer::protocol
