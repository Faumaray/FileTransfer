#pragma once

#include <ftu/platform/file_descriptor.hpp>
#include <ftu/transfer/transport/connection.hpp>

#include <cstdint>
#include <span>
#include <string_view>
#include <sys/types.h>
#include <sys/un.h>
#include <utility>

namespace ftu::transfer::transport::unix_socket
{
	class Connection final : public transport::Connection
	{
	public:
		explicit Connection(platform::FileDescriptor socket) :
			m_socket(std::move(socket))
		{
		}

		static platform::FileDescriptor openSocket();
		static sockaddr_un address(std::string_view path);

		[[nodiscard]] int nativeHandle() const noexcept override { return m_socket.get(); }

		transport::IoResult readSome(std::span<std::uint8_t> destination) override;
		transport::IoResult writeSome(std::span<const std::uint8_t> source) override;
		[[nodiscard]] uid_t peerUid() const;

	private:
		platform::FileDescriptor m_socket;
	};
} // namespace ftu::transfer::transport::unix_socket
