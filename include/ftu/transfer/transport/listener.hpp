#pragma once

#include <ftu/transfer/transport/connection.hpp>

#include <memory>

namespace ftu::transfer::transport
{
	class Listener
	{
	public:
		virtual ~Listener() = default;
		[[nodiscard]] virtual int nativeHandle() const noexcept = 0;
		virtual std::unique_ptr<Connection> acceptOne() = 0;
	};
} // namespace ftu::transfer::transport
