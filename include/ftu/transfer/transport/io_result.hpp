#pragma once

#include <cstddef>

namespace ftu::transfer::transport
{
	enum class IoStatus
	{
		Data,
		WouldBlock,
		End
	};

	struct IoResult
	{
		IoStatus status;
		std::size_t count = 0;
	};
} // namespace ftu::transfer::transport
