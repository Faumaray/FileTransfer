#pragma once

#include <ftu/checksum/crc64.hpp>
#include <ftu/transfer/protocol/file_metadata.hpp>

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace ftu::transfer::storage
{
	class SessionStore;

	class Session
	{
	public:
		Session(SessionStore& store, protocol::FileMetadata metadata);
		~Session();
		Session(const Session&) = delete;
		Session& operator=(const Session&) = delete;

		[[nodiscard]] const protocol::FileMetadata& metadata() const noexcept { return m_metadata; }

		[[nodiscard]] std::uint64_t offset() const noexcept { return m_offset; }

		[[nodiscard]] std::uint64_t crc() const noexcept { return m_crc.value(); }

		[[nodiscard]] bool completed() const noexcept { return m_completed; }

		[[nodiscard]] const std::string& outputName() const noexcept { return m_output_name; }

		void append(std::uint64_t offset, std::span<const std::uint8_t> data);
		void reset();
		void finish();

	private:
		void load();
		[[nodiscard]] std::string createManifest() const;
		[[nodiscard]] std::string readManifest() const;
		void adoptPublished();
		void openPartial();
		void publish() const;
		void removePartial() const;
		[[nodiscard]] std::filesystem::path outputPath() const;

		SessionStore& m_store;
		protocol::FileMetadata m_metadata;
		std::string m_key;
		std::filesystem::path m_directory;
		std::filesystem::path m_part;
		std::string m_output_name;
		std::uint64_t m_offset = 0;
		checksum::Crc64 m_crc;
		bool m_completed = false;
	};
} // namespace ftu::transfer::storage
