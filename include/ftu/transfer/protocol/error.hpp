#pragma once

#include "ftu/transfer/protocol/error_code.hpp"

#include <stdexcept>
#include <string>

namespace ftu::transfer::protocol
{
	class Error : public std::runtime_error
	{
	public:
		Error(ErrorCode code, std::string message) :
			std::runtime_error(message),
			m_code(code)
		{
		}

		[[nodiscard]] ErrorCode code() const noexcept { return m_code; }

	private:
		ErrorCode m_code;
	};

} // namespace ftu::transfer::protocol
