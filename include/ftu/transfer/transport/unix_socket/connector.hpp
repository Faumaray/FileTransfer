#pragma once

#include "ftu/transfer/transport/connector.hpp"

#include <chrono>
#include <string>
#include <utility>

namespace ftu::transfer::transport::unix_socket
{
	class Connector final : public transport::Connector
	{
	public:
		explicit Connector(std::string path) :
			m_socket_path(std::move(path))
		{
		}

		std::unique_ptr<transport::Connection> connect(std::chrono::milliseconds timeout) override;

	private:
		std::string m_socket_path;
	};

} // namespace ftu::transfer::transport::unix_socket
