#pragma once

#include <cstddef>

namespace ftu::transfer::protocol
{
	constexpr std::size_t FILE_CHUNK_SIZE = 64U * 1024U;
	constexpr std::size_t MESSAGE_HEADER_SIZE = 32;
	constexpr std::size_t MAX_PAYLOAD_SIZE = FILE_CHUNK_SIZE + 8;
	constexpr std::size_t MAX_FILENAME_SIZE = 255;

} // namespace ftu::transfer::protocol
