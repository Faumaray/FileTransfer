#include <ftu/platform/system_error.hpp>

#include <cerrno>
#include <string>
#include <system_error>

namespace ftu::platform
{
	void systemFail(std::string_view operation)
	{
		const int error = errno;
		throw std::system_error(error, std::generic_category(), std::string(operation));
	}
} // namespace ftu::platform
