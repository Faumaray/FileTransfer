#pragma once

#include <ftu/transfer/transport/connection.hpp>

#include <chrono>
#include <memory>

namespace ftu::transfer::transport
{
	class Connector
	{
	public:
		virtual ~Connector() = default;
		virtual std::unique_ptr<Connection> connect(std::chrono::milliseconds timeout) = 0;
	};
} // namespace ftu::transfer::transport
