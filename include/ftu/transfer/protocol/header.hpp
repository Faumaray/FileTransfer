#pragma once

#include "ftu/transfer/protocol/message_type.hpp"

#include <cstdint>

namespace ftu::transfer::protocol
{
	struct Header
	{
		MessageType type;
		std::uint32_t payload_size;
		std::uint64_t payload_crc64;
	};
} // namespace ftu::transfer::protocol
