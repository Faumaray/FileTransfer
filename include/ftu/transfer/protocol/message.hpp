#pragma once

#include "ftu/transfer/protocol/error.hpp"
#include "ftu/transfer/protocol/error_code.hpp"
#include "ftu/transfer/protocol/header.hpp"
#include "ftu/transfer/protocol/message_type.hpp"
#include "ftu/transfer/protocol/progress.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ftu::transfer::protocol
{
	struct Message
	{
		MessageType type;
		std::vector<std::uint8_t> payload;

		static Message progress(MessageType type, std::uint64_t offset, std::uint64_t crc);
		static Message error(ErrorCode code, std::string_view text);
		static Message done(std::string_view saved_name);

		[[nodiscard]] Progress asProgress(MessageType expected) const;
		[[nodiscard]] Error asError() const;
		[[nodiscard]] std::string asSavedName() const;

		[[nodiscard]] std::vector<std::uint8_t> encode() const;
		static Header decodeHeader(std::span<const std::uint8_t> bytes);
		static Message decode(std::span<const std::uint8_t> bytes);
	};
} // namespace ftu::transfer::protocol
