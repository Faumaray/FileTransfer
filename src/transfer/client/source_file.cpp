#include <ftu/transfer/client/source_file.hpp>

#include <ftu/checksum/crc64.hpp>

#include <stdexcept>
#include <system_error>

namespace ftu::transfer::client
{
	SourceFile::SourceFile(std::filesystem::path path) :
		m_path(std::move(path)),
		m_size(std::filesystem::file_size(m_path)),
		m_modified(std::filesystem::last_write_time(m_path)),
		m_stream(m_path, std::ios::binary)
	{
		if (!m_stream)
		{
			throw std::runtime_error("cannot open source " + m_path.string());
		}
	}

	std::uint64_t SourceFile::crc64(std::uint64_t length) const
	{
		return checksum::Crc64::compute(m_path, length);
	}

	void SourceFile::read(std::span<std::uint8_t> destination, std::uint64_t offset)
	{
		m_stream.clear();
		m_stream.seekg(static_cast<std::streamoff>(offset));
		if (!m_stream.read(
				reinterpret_cast<char*>(destination.data()), static_cast<std::streamsize>(destination.size())
			))
		{
			throw std::runtime_error("unexpected end of source file (file changed or truncated)");
		}
	}

	void SourceFile::checkUnchanged() const
	{
		std::error_code sizeError;
		std::error_code timeError;
		const auto size = std::filesystem::file_size(m_path, sizeError);
		const auto modified = std::filesystem::last_write_time(m_path, timeError);
		if (sizeError || timeError || size != m_size || modified != m_modified)
		{
			throw std::runtime_error(
				"source file changed during transfer; run client again after writers stop"
			);
		}
	}
} // namespace ftu::transfer::client
