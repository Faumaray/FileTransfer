#include <ftu/transfer/storage/session.hpp>

#include <ftu/platform/file_descriptor.hpp>
#include <ftu/transfer/protocol/constants.hpp>
#include <ftu/transfer/protocol/decoder.hpp>
#include <ftu/transfer/protocol/encoder.hpp>
#include <ftu/transfer/protocol/error.hpp>
#include <ftu/transfer/protocol/message.hpp>
#include <ftu/transfer/storage/session_store.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace ftu::transfer::storage
{
	namespace
	{
		std::string newOutputName()
		{
			static std::atomic<std::int64_t> last {0};
			const std::int64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
										 std::chrono::system_clock::now().time_since_epoch()
			)
										 .count();
			auto previous = last.load();
			auto candidate = std::max(now, previous + 1);
			while (!last.compare_exchange_weak(previous, candidate))
			{
				candidate = std::max(now, previous + 1);
			}
			const std::time_t seconds = candidate / 1'000'000'000;
			std::tm utc {};
			::gmtime_r(&seconds, &utc);
			std::ostringstream name;
			name << std::put_time(&utc, "%Y%m%d_%H%M%S_") << std::setfill('0') << std::setw(9)
				 << candidate % 1'000'000'000 << ".hex";
			return name.str();
		}

		void validateOutputName(const std::string& name)
		{
			if (name.size() != 29 || name[8] != '_' || name[15] != '_' || name.substr(25) != ".hex")
			{
				throw protocol::Error(protocol::ErrorCode::Storage, "invalid output name in manifest");
			}
			for (std::size_t i = 0; i < 25; ++i)
			{
				if (i != 8 && i != 15 && (name[i] < '0' || name[i] > '9'))
				{
					throw protocol::Error(
						protocol::ErrorCode::Storage, "invalid output timestamp in manifest"
					);
				}
			}
		}

		std::vector<std::uint8_t> readSmallFile(const std::filesystem::path& path, std::uintmax_t limit)
		{
			const auto size = std::filesystem::file_size(path);
			if (size > limit)
			{
				throw std::runtime_error("metadata file too large");
			}
			std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
			std::ifstream file(path, std::ios::binary);
			if (!file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size)))
			{
				throw std::runtime_error("cannot read " + path.string());
			}
			return bytes;
		}

		void writeAtomically(const std::filesystem::path& path, std::span<const std::uint8_t> bytes)
		{
			auto temporary = path;
			temporary += ".tmp";
			{
				std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
				if (!file.write(
							 reinterpret_cast<const char*>(bytes.data()),
							 static_cast<std::streamsize>(bytes.size())
					)
						 .flush())
				{
					throw std::runtime_error("cannot write " + temporary.string());
				}
			}
			platform::FileDescriptor::sync(temporary);
			std::filesystem::rename(temporary, path);
			platform::FileDescriptor::sync(path.parent_path());
		}
	} // namespace

	Session::Session(SessionStore& store, protocol::FileMetadata metadata) :
		m_store(store),
		m_metadata(std::move(metadata)),
		m_key(m_metadata.uuid.toString()),
		m_directory(m_store.m_state_directory / m_key),
		m_part(m_directory / "data.part")
	{
		m_metadata.validate();
		m_store.claim(m_key, m_metadata.size);
		try
		{
			load();
		}
		catch (...)
		{
			m_store.release(m_key);
			throw;
		}
	}

	Session::~Session()
	{
		m_store.release(m_key);
	}

	void Session::load()
	{
		if (std::filesystem::create_directory(m_directory))
		{
			std::filesystem::permissions(m_directory, std::filesystem::perms::owner_all);
			platform::FileDescriptor::sync(m_store.m_state_directory);
		}
		m_output_name = std::filesystem::exists(m_directory / "manifest") ? readManifest() : createManifest();
		if (std::filesystem::exists(std::filesystem::symlink_status(outputPath())))
		{
			adoptPublished();
		}
		else
		{
			openPartial();
		}
	}

	std::string Session::createManifest() const
	{
		std::string outputName;
		while (
			std::filesystem::exists(std::filesystem::symlink_status(m_store.m_output_directory / outputName)))
		{
			outputName = newOutputName();
		}
		const auto encoded = m_metadata.encode();
		protocol::Encoder out;
		out.writeInteger<std::uint32_t>(static_cast<std::uint32_t>(encoded.size()));
		out.writeBytes(encoded);
		out.writeString(outputName);
		writeAtomically(
			m_directory / "manifest",
			protocol::Message {
				.type = protocol::MessageType::Hello, .payload = std::move(out).releaseBuffer()
			}
				.encode()
		);
		return outputName;
	}

	std::string Session::readManifest() const
	{
		const auto frame = protocol::Message::decode(readSmallFile(m_directory / "manifest", 4096));
		if (frame.type != protocol::MessageType::Hello)
		{
			throw protocol::Error(protocol::ErrorCode::Storage, "invalid manifest type");
		}
		protocol::Decoder in(frame.payload);
		const auto metadataSize = in.readInteger<std::uint32_t>();
		if (metadataSize > 1024)
		{
			throw protocol::Error(protocol::ErrorCode::Storage, "invalid manifest metadata size");
		}
		const auto saved = protocol::FileMetadata::decode(in.readBytes(metadataSize));
		auto outputName = in.readString(64);
		in.requireEnd();
		validateOutputName(outputName);
		if (saved != m_metadata)
		{
			throw protocol::Error(protocol::ErrorCode::Collision, "UUID collision: metadata differs");
		}
		return outputName;
	}

	void Session::adoptPublished()
	{
		const auto published = outputPath();
		if (!std::filesystem::is_regular_file(std::filesystem::symlink_status(published)) ||
			std::filesystem::file_size(published) != m_metadata.size ||
			checksum::Crc64::compute(published, m_metadata.size) != m_metadata.crc)
		{
			throw protocol::Error(
				protocol::ErrorCode::Storage, "published file was modified; refusing to overwrite it"
			);
		}
		platform::FileDescriptor::sync(published);
		platform::FileDescriptor::sync(m_store.m_output_directory);
		m_completed = true;
		m_offset = m_metadata.size;
		m_crc = checksum::Crc64(m_metadata.crc);
		removePartial();
	}

	void Session::openPartial()
	{
		if (!std::filesystem::exists(m_part))
		{
			platform::FileDescriptor::open(m_part, O_WRONLY | O_CREAT | O_NOFOLLOW, 0600);
			platform::FileDescriptor::sync(m_directory);
		}
		m_offset = std::filesystem::file_size(m_part);
		if (m_offset > m_metadata.size)
		{
			reset();
		}
		m_crc = checksum::Crc64(checksum::Crc64::compute(m_part, m_offset));
	}

	void Session::append(std::uint64_t offset, std::span<const std::uint8_t> data)
	{
		if (m_completed || offset != m_offset || data.empty() || data.size() > protocol::FILE_CHUNK_SIZE ||
			data.size() > m_metadata.size - m_offset)
		{
			throw protocol::Error(protocol::ErrorCode::Protocol, "invalid DATA offset or size");
		}
		std::ofstream part(m_part, std::ios::binary | std::ios::app);
		if (!part.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()))
				 .flush())
		{
			throw std::runtime_error("cannot write " + m_part.string());
		}
		m_crc.update(data);
		m_offset += data.size();
	}

	void Session::reset()
	{
		if (m_completed)
		{
			throw protocol::Error(protocol::ErrorCode::Protocol, "cannot reset a completed session");
		}
		std::filesystem::resize_file(m_part, 0);
		platform::FileDescriptor::sync(m_part);
		m_offset = 0;
		m_crc = checksum::Crc64 {};
	}

	void Session::finish()
	{
		if (m_completed)
		{
			return;
		}
		if (m_offset != m_metadata.size)
		{
			throw protocol::Error(protocol::ErrorCode::Protocol, "FINISH before all bytes arrived");
		}
		if (m_crc.value() != m_metadata.crc)
		{
			reset();
			throw protocol::Error(protocol::ErrorCode::Integrity, "whole-file CRC64 mismatch; partial reset");
		}
		platform::FileDescriptor::sync(m_part);
		if (std::filesystem::file_size(m_part) != m_metadata.size ||
			checksum::Crc64::compute(m_part, m_metadata.size) != m_metadata.crc)
		{
			reset();
			throw protocol::Error(
				protocol::ErrorCode::Integrity, "stored-file CRC64 mismatch; partial reset"
			);
		}
		publish();
		platform::FileDescriptor::sync(m_store.m_output_directory);
		m_completed = true;
		removePartial();
	}

	void Session::publish() const
	{
		const auto published = outputPath();
		std::error_code error;
		std::filesystem::create_hard_link(m_part, published, error);
		if (!error)
		{
			return;
		}
		if (error != std::errc::file_exists)
		{
			throw std::filesystem::filesystem_error("publish verified file", m_part, published, error);
		}
		if (std::filesystem::is_symlink(published) || !std::filesystem::equivalent(m_part, published))
		{
			throw protocol::Error(
				protocol::ErrorCode::Storage, "output name already exists; no file was overwritten"
			);
		}
	}

	void Session::removePartial() const
	{
		std::filesystem::remove(m_part);
		platform::FileDescriptor::sync(m_directory);
	}

	std::filesystem::path Session::outputPath() const
	{
		return m_store.m_output_directory / m_output_name;
	}
} // namespace ftu::transfer::storage
