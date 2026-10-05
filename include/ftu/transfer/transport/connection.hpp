#pragma once

#include <ftu/transfer/transport/io_result.hpp>

#include <chrono>
#include <cstdint>
#include <span>

namespace ftu::transfer::transport
{
	class Connection
	{
	public:
		virtual ~Connection() = default;
		[[nodiscard]] virtual int nativeHandle() const noexcept = 0;
		virtual IoResult readSome(std::span<std::uint8_t> destination) = 0;
		virtual IoResult writeSome(std::span<const std::uint8_t> source) = 0;

		void waitReady(std::int16_t events, std::chrono::steady_clock::time_point deadline) const;
	};
} // namespace ftu::transfer::transport
