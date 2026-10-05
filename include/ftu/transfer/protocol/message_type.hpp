#pragma once

#include <cstdint>

namespace ftu::transfer::protocol
{
	enum class MessageType : std::uint16_t
	{
		Hello = 1,
		Ready = 2,
		Data = 3,
		Ack = 4,
		Finish = 5,
		Done = 6,
		Error = 7,
		Reset = 8
	};

} // namespace ftu::transfer::protocol
