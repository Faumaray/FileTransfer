#include <ftu/platform/file_descriptor.hpp>

#include <ftu/platform/system_error.hpp>

#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ftu::platform
{
	FileDescriptor::~FileDescriptor()
	{
		if (m_descriptor >= 0)
		{
			::close(m_descriptor);
		}
	}

	FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept
	{
		if (this != &other)
		{
			if (m_descriptor >= 0)
			{
				::close(m_descriptor);
			}
			m_descriptor = std::exchange(other.m_descriptor, -1);
		}
		return *this;
	}

	FileDescriptor FileDescriptor::open(const std::filesystem::path& path, int flags, mode_t mode)
	{
		FileDescriptor fd(::open(path.c_str(), flags | O_CLOEXEC, mode));
		if (!fd)
		{
			systemFail("open " + path.string());
		}
		return fd;
	}

	void FileDescriptor::sync(const std::filesystem::path& path)
	{
		const auto fd = open(path, O_RDONLY);
		while (::fsync(fd.get()) < 0)
		{
			if (errno != EINTR)
			{
				systemFail("fsync " + path.string());
			}
		}
	}

	bool FileDescriptor::tryLock() const
	{
		if (::flock(m_descriptor, LOCK_EX | LOCK_NB) == 0)
		{
			return true;
		}
		if (errno == EWOULDBLOCK)
		{
			return false;
		}
		systemFail("flock");
	}

	bool FileDescriptor::ownedPrivately() const
	{
		struct stat status {};
		check(::fstat(m_descriptor, &status), "fstat");
		return status.st_uid == ::geteuid() && (status.st_mode & 0077) == 0;
	}
} // namespace ftu::platform
