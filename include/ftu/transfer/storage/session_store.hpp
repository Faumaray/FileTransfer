#pragma once

#include <ftu/platform/file_descriptor.hpp>
#include <ftu/transfer/protocol/file_metadata.hpp>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>

namespace ftu::transfer::storage
{
	class Session;

	class SessionStore
	{
	public:
		SessionStore(std::filesystem::path output_directory, std::uint64_t max_file_size);
		SessionStore(const SessionStore&) = delete;
		SessionStore& operator=(const SessionStore&) = delete;

		std::unique_ptr<Session> acquire(protocol::FileMetadata metadata);

	private:
		friend class Session;
		void claim(const std::string& key, std::uint64_t size);
		void release(std::string_view key) noexcept;

		std::filesystem::path m_output_directory;
		std::filesystem::path m_state_directory;
		platform::FileDescriptor m_lock;
		std::uint64_t m_max_file_size;
		std::mutex m_mutex;
		std::set<std::string, std::less<>> m_active;
	};
} // namespace ftu::transfer::storage
