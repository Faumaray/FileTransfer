#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

namespace ftu::checksum
{
	class Crc64
	{
	public:
		explicit Crc64(std::uint64_t initial = 0) noexcept :
			m_state(initial)
		{
		}

		void update(std::span<const std::uint8_t> data) noexcept;

		[[nodiscard]] std::uint64_t value() const noexcept { return m_state; }

		static std::uint64_t compute(std::span<const std::uint8_t> data) noexcept;
		static std::uint64_t compute(const std::filesystem::path& file, std::uint64_t length);

	private:
		std::uint64_t m_state;
	};
} // namespace ftu::checksum
