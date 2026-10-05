#include "ftu/transfer/transport/unix_socket/connection.hpp"
#include "ftu/platform/system_error.hpp"
#include "ftu/transfer/transport/disconnected.hpp"

#include <algorithm>
#include <cerrno>
#include <iterator>
#include <stdexcept>
#include <sys/socket.h>

namespace ftu::transfer::transport::unix_socket
{
	platform::FileDescriptor Connection::openSocket()
	{
		return platform::FileDescriptor(
			platform::check(
				::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0), "socket AF_UNIX"
			)
		);
	}

	sockaddr_un Connection::address(std::string_view path)
	{
		sockaddr_un address {};
		address.sun_family = AF_UNIX;
		if (path.empty() || path.size() >= sizeof(address.sun_path) ||
			path.find('\0') != std::string_view::npos)
		{
			throw std::runtime_error("Unix socket path must contain 1..107 non-NUL bytes");
		}
		std::ranges::copy(path, std::begin(address.sun_path));
		return address;
	}

	uid_t Connection::peerUid() const
	{
		ucred peer {};
		socklen_t size = sizeof(peer);
		platform::check(::getsockopt(m_socket.get(), SOL_SOCKET, SO_PEERCRED, &peer, &size), "SO_PEERCRED");
		return peer.uid;
	}

	transport::IoResult Connection::readSome(std::span<std::uint8_t> destination)
	{
		if (destination.empty())
		{
			return {.status = transport::IoStatus::Data, .count = 0};
		}
		for (;;)
		{
			const auto n = ::recv(m_socket.get(), destination.data(), destination.size(), 0);
			if (n > 0)
			{
				return {.status = transport::IoStatus::Data, .count = static_cast<std::size_t>(n)};
			}
			if (n == 0)
			{
				return {.status = transport::IoStatus::End, .count = 0};
			}
			if (errno == EINTR)
			{
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return {.status = transport::IoStatus::WouldBlock, .count = 0};
			}
			platform::systemFail("recv");
		}
	}

	transport::IoResult Connection::writeSome(std::span<const std::uint8_t> source)
	{
		if (source.empty())
		{
			return {.status = transport::IoStatus::Data, .count = 0};
		}
		for (;;)
		{
			const auto n = ::send(m_socket.get(), source.data(), source.size(), MSG_NOSIGNAL);
			if (n > 0)
			{
				return {.status = transport::IoStatus::Data, .count = static_cast<std::size_t>(n)};
			}
			if (n == 0)
			{
				throw transport::Disconnected("send returned zero");
			}
			if (errno == EINTR)
			{
				continue;
			}
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return {.status = transport::IoStatus::WouldBlock, .count = 0};
			}
			platform::systemFail("send");
		}
	}
} // namespace ftu::transfer::transport::unix_socket
