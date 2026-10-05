#pragma once

#include <cstdint>

namespace ftu::transfer::server
{
	struct Completion
	{
		std::uint64_t connection_token;
		std::uint32_t ready_events;
	};
} // namespace ftu::transfer::server
