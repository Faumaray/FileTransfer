#pragma once

#include <concepts>
#include <string_view>

namespace ftu::platform
{
	[[noreturn]] void systemFail(std::string_view operation);

	template <std::signed_integral T>
	T check(T result, std::string_view operation)
	{
		if (result < 0)
		{
			systemFail(operation);
		}
		return result;
	}
} // namespace ftu::platform
