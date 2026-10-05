#include "ftu/transfer/protocol/file_metadata.hpp"
#include "ftu/transfer/protocol/constants.hpp"
#include "ftu/transfer/protocol/decoder.hpp"
#include "ftu/transfer/protocol/encoder.hpp"
#include "ftu/transfer/protocol/error.hpp"

#include <algorithm>

namespace ftu::transfer::protocol
{
	std::vector<std::uint8_t> FileMetadata::encode() const
	{
		Encoder out;
		out.writeBytes(uuid.bytes);
		out.writeInteger<std::uint64_t>(size);
		out.writeInteger<std::uint64_t>(crc);
		out.writeString(name);
		return std::move(out).releaseBuffer();
	}

	FileMetadata FileMetadata::decode(std::span<const std::uint8_t> bytes)
	{
		Decoder in(bytes);
		FileMetadata metadata;
		const auto id = in.readBytes(metadata.uuid.bytes.size());
		std::ranges::copy(id, metadata.uuid.bytes.begin());
		metadata.size = in.readInteger<std::uint64_t>();
		metadata.crc = in.readInteger<std::uint64_t>();
		metadata.name = in.readString(MAX_FILENAME_SIZE);
		in.requireEnd();
		metadata.validate();
		return metadata;
	}

	void FileMetadata::validate() const
	{
		if (uuid != SessionId::of(crc, name, size))
		{
			throw Error(ErrorCode::Protocol, "UUID does not match file metadata");
		}
	}

	void FileMetadata::validateBasename(std::string_view name)
	{
		if (name.empty() || name.size() > MAX_FILENAME_SIZE || name == "." || name == ".." ||
			name.find('/') != std::string_view::npos || name.find('\0') != std::string_view::npos)
		{
			throw Error(ErrorCode::Protocol, "invalid basename");
		}
	}
} // namespace ftu::transfer::protocol
