#include <ftu/transfer/transport/unix_socket/connector.hpp>

#include <ftu/platform/system_error.hpp>
#include <ftu/transfer/transport/unix_socket/connection.hpp>

#include <cerrno>
#include <chrono>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace ftu::transfer::transport::unix_socket
{
	std::unique_ptr<transport::Connection> Connector::connect(std::chrono::milliseconds timeout)
	{
		const auto address = Connection::address(m_socket_path);
		auto connection = std::make_unique<Connection>(Connection::openSocket());
		const int fd = connection->nativeHandle();
		if (::connect(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0)
		{
			if (errno != EINPROGRESS)
			{
				platform::systemFail("connect " + m_socket_path);
			}
			connection->waitReady(POLLOUT, std::chrono::steady_clock::now() + timeout);
			int error = 0;
			socklen_t size = sizeof(error);
			platform::check(::getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &size), "SO_ERROR");
			if (error != 0)
			{
				errno = error;
				platform::systemFail("connect");
			}
		}
		if (connection->peerUid() != ::geteuid())
		{
			throw std::runtime_error("server UID differs from client UID");
		}
		return connection;
	}
} // namespace ftu::transfer::transport::unix_socket
