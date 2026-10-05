#pragma once

#include <cstdint>

namespace ftu::transfer::protocol
{
	struct Progress
	{
		std::uint64_t byte_offset;
		std::uint64_t prefix_crc64;
	};
} // namespace ftu::transfer::protocol
