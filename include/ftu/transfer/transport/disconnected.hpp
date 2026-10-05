#pragma once

#include <stdexcept>

namespace ftu::transfer::transport
{
	class Disconnected : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};
} // namespace ftu::transfer::transport
