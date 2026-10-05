#pragma once

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

namespace ftu::transfer::protocol
{
	struct SessionId
	{
		std::array<std::uint8_t, 16> bytes {};

		static SessionId of(std::uint64_t file_crc, std::string_view basename, std::uint64_t size);
		[[nodiscard]] std::string toString() const;
		bool operator==(const SessionId&) const = default;

		friend std::ostream& operator<<(std::ostream& out, const SessionId& id)
		{
			return out << id.toString();
		}
	};
} // namespace ftu::transfer::protocol
