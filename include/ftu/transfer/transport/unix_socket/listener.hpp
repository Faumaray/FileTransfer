#pragma once

#include <memory>

#include "ftu/platform/file_descriptor.hpp"
#include "ftu/transfer/transport/listener.hpp"

#include <cstdint>
#include <string>

namespace ftu::transfer::transport::unix_socket
{
	class Listener final : public transport::Listener
	{
	public:
		explicit Listener(std::string path);
		~Listener() override;

		int nativeHandle() const noexcept override { return m_socket.get(); }

		std::unique_ptr<transport::Connection> acceptOne() override;

	private:
		std::string m_socket_path;
		platform::FileDescriptor m_lock;
		platform::FileDescriptor m_socket;
		std::uint64_t m_inode = 0;
		std::uint64_t m_device = 0;
	};

} // namespace ftu::transfer::transport::unix_socket
