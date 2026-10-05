#pragma once

#include "ftu/transfer/protocol/session_id.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ftu::transfer::protocol
{
	struct FileMetadata
	{
		SessionId uuid {};
		std::uint64_t size = 0;
		std::uint64_t crc = 0;
		std::string name;

		[[nodiscard]] std::vector<std::uint8_t> encode() const;
		static FileMetadata decode(std::span<const std::uint8_t> bytes);
		void validate() const;
		static void validateBasename(std::string_view name);

		bool operator==(const FileMetadata&) const = default;
	};
} // namespace ftu::transfer::protocol
