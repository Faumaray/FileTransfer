#include "ftu/checksum/crc64.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>

namespace ftu::checksum
{
	namespace
	{
		constexpr std::array<std::uint64_t, 256> makeTable()
		{
			std::array<std::uint64_t, 256> table {};
			for (std::size_t i = 0; i < table.size(); ++i)
			{
				std::uint64_t crc = static_cast<std::uint64_t>(i) << 56U;
				for (int bit = 0; bit < 8; ++bit)
				{
					crc = (crc & (1ULL << 63U)) != 0 ? (crc << 1U) ^ 0x42'F0'E1'EB'A9'EA'36'93ULL : crc << 1U;
				}
				table[i] = crc;
			}
			return table;
		}

		constexpr auto TABLE = makeTable();
	} // namespace

	void Crc64::update(std::span<const std::uint8_t> data) noexcept
	{
		for (const auto byte : data)
		{
			m_state = TABLE[static_cast<std::uint8_t>((m_state >> 56U) ^ byte)] ^ (m_state << 8U);
		}
	}

	std::uint64_t Crc64::compute(std::span<const std::uint8_t> data) noexcept
	{
		Crc64 crc;
		crc.update(data);
		return crc.value();
	}

	std::uint64_t Crc64::compute(const std::filesystem::path& file, std::uint64_t length)
	{
		std::ifstream in(file, std::ios::binary);
		std::array<std::uint8_t, 64U * 1024U> buffer {};
		Crc64 crc;
		for (std::uint64_t done = 0; done < length;)
		{
			const auto chunk =
				static_cast<std::size_t>(std::min<std::uint64_t>(buffer.size(), length - done));
			if (!in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(chunk)))
			{
				throw std::runtime_error("cannot read " + file.string() + " (changed or truncated)");
			}
			crc.update(std::span(buffer).first(chunk));
			done += chunk;
		}
		return crc.value();
	}
} // namespace ftu::checksum
