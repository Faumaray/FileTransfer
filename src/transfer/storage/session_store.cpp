#include <ftu/transfer/storage/session_store.hpp>

#include <ftu/transfer/protocol/error.hpp>
#include <ftu/transfer/storage/session.hpp>

#include <fcntl.h>
#include <stdexcept>

namespace ftu::transfer::storage
{
	SessionStore::SessionStore(std::filesystem::path output_directory, std::uint64_t max_file_size) :
		m_output_directory(std::move(output_directory)),
		m_state_directory(m_output_directory / ".FTU-state"),
		m_max_file_size(max_file_size)
	{
		if (std::filesystem::create_directory(m_state_directory))
		{
			std::filesystem::permissions(m_state_directory, std::filesystem::perms::owner_all);
			platform::FileDescriptor::sync(m_output_directory);
		}
		if (!platform::FileDescriptor::open(m_state_directory, O_RDONLY | O_DIRECTORY | O_NOFOLLOW)
				 .ownedPrivately())
		{
			throw std::runtime_error(
				"directory must be owned by current user and private (0700): " + m_state_directory.string()
			);
		}
		m_lock =
			platform::FileDescriptor::open(m_state_directory / "server.lock", O_RDWR | O_CREAT | O_NOFOLLOW);
		if (!m_lock.tryLock())
		{
			throw std::runtime_error("another server is using this executable directory");
		}
	}

	std::unique_ptr<Session> SessionStore::acquire(protocol::FileMetadata metadata)
	{
		return std::make_unique<Session>(*this, std::move(metadata));
	}

	void SessionStore::claim(const std::string& key, std::uint64_t size)
	{
		if (m_max_file_size != 0 && size > m_max_file_size)
		{
			throw protocol::Error(protocol::ErrorCode::Limit, "file size exceeds server limit");
		}
		const std::lock_guard lock(m_mutex);
		if (!m_active.insert(key).second)
		{
			throw protocol::Error(protocol::ErrorCode::Busy, "session " + key + " is already active");
		}
	}

	void SessionStore::release(std::string_view key) noexcept
	{
		const std::lock_guard lock(m_mutex);
		const auto found = m_active.find(key);
		if (found != m_active.end())
		{
			m_active.erase(found);
		}
	}
} // namespace ftu::transfer::storage
