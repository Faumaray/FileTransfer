#include "ftu/transfer/transport/unix_socket/listener.hpp"
#include "ftu/logging/logger.hpp"
#include "ftu/platform/system_error.hpp"
#include "ftu/transfer/transport/unix_socket/connection.hpp"

#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ftu::transfer::transport::unix_socket
{
	namespace
	{
		void removeStaleSocket(const std::string& path, const sockaddr_un& address)
		{
			struct stat status {};
			if (::lstat(path.c_str(), &status) < 0)
			{
				if (errno != ENOENT)
				{
					platform::systemFail("lstat socket");
				}
				return;
			}
			if (!S_ISSOCK(status.st_mode) || status.st_uid != ::geteuid())
			{
				throw std::runtime_error("refusing to unlink a non-socket or foreign socket");
			}
			const auto probe = Connection::openSocket();
			if (::connect(probe.get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0)
			{
				throw std::runtime_error("socket is served by another process");
			}
			if (errno != ECONNREFUSED && errno != ENOENT)
			{
				platform::systemFail("socket is not demonstrably stale");
			}
			std::filesystem::remove(path);
		}
	} // namespace

	Listener::Listener(std::string path) :
		m_socket_path(std::move(path))
	{
		const auto address = Connection::address(m_socket_path);
		m_lock = platform::FileDescriptor::open(m_socket_path + ".lock", O_RDWR | O_CREAT | O_NOFOLLOW);
		if (!m_lock.tryLock())
		{
			throw std::runtime_error("another server owns the socket lock");
		}
		removeStaleSocket(m_socket_path, address);
		m_socket = Connection::openSocket();
		platform::check(
			::bind(m_socket.get(), reinterpret_cast<const sockaddr*>(&address), sizeof(address)), "bind"
		);
		try
		{
			std::filesystem::permissions(
				m_socket_path, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write
			);
			struct stat status {};
			platform::check(::lstat(m_socket_path.c_str(), &status), "lstat bound socket");
			m_inode = status.st_ino;
			m_device = status.st_dev;
			platform::check(::listen(m_socket.get(), 128), "listen");
		}
		catch (...)
		{
			::unlink(m_socket_path.c_str());
			throw;
		}
	}

	Listener::~Listener()
	{
		struct stat status {};
		if (::lstat(m_socket_path.c_str(), &status) == 0 && status.st_ino == m_inode &&
			status.st_dev == m_device)
		{
			::unlink(m_socket_path.c_str());
		}
	}

	std::unique_ptr<transport::Connection> Listener::acceptOne()
	{
		for (;;)
		{
			const int fd = ::accept4(m_socket.get(), nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
			if (fd < 0)
			{
				if (errno == EINTR || errno == ECONNABORTED)
				{
					continue;
				}
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					return nullptr;
				}
				platform::systemFail("accept4");
			}
			auto connection = std::make_unique<Connection>(platform::FileDescriptor(fd));
			if (connection->peerUid() != ::geteuid())
			{
				logging::Logger::warn("SERVER REJECT foreign UID");
				continue;
			}
			return connection;
		}
	}
} // namespace ftu::transfer::transport::unix_socket
