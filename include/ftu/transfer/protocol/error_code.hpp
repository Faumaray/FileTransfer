#pragma once

#include <cstdint>

namespace ftu::transfer::protocol
{
	enum class ErrorCode : std::uint8_t
	{
		Busy = 1,
		Protocol = 2,
		Integrity = 3,
		Storage = 4,
		Collision = 5,
		Limit = 6
	};

} // namespace ftu::transfer::protocol
