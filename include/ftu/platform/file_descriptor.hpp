#pragma once

#include <filesystem>
#include <sys/types.h>
#include <utility>

namespace ftu::platform
{
	class FileDescriptor
	{
	public:
		explicit FileDescriptor(int fd = -1) noexcept :
			m_descriptor(fd)
		{
		}

		~FileDescriptor();
		FileDescriptor(const FileDescriptor&) = delete;
		FileDescriptor& operator=(const FileDescriptor&) = delete;

		FileDescriptor(FileDescriptor&& other) noexcept :
			m_descriptor(std::exchange(other.m_descriptor, -1))
		{
		}

		FileDescriptor& operator=(FileDescriptor&& other) noexcept;

		static FileDescriptor open(const std::filesystem::path& path, int flags, mode_t mode = 0600);
		static void sync(const std::filesystem::path& path);

		[[nodiscard]] int get() const noexcept { return m_descriptor; }

		explicit operator bool() const noexcept { return m_descriptor >= 0; }

		[[nodiscard]] bool tryLock() const;
		[[nodiscard]] bool ownedPrivately() const;

	private:
		int m_descriptor;
	};
} // namespace ftu::platform
