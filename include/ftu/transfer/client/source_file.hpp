#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>

namespace ftu::transfer::client
{
	class SourceFile
	{
	public:
		explicit SourceFile(std::filesystem::path path);

		[[nodiscard]] const std::filesystem::path& path() const noexcept { return m_path; }

		[[nodiscard]] std::uint64_t size() const noexcept { return m_size; }

		[[nodiscard]] std::uint64_t crc64(std::uint64_t length) const;
		void read(std::span<std::uint8_t> destination, std::uint64_t offset);
		void checkUnchanged() const;

	private:
		std::filesystem::path m_path;
		std::uint64_t m_size;
		std::filesystem::file_time_type m_modified;
		std::ifstream m_stream;
	};
} // namespace ftu::transfer::client
