#include <ftu/transfer/protocol/session_id.hpp>

#include <ftu/checksum/crc64.hpp>
#include <ftu/transfer/protocol/encoder.hpp>
#include <ftu/transfer/protocol/file_metadata.hpp>
namespace ftu::transfer::protocol
{
	SessionId SessionId::of(std::uint64_t file_crc, std::string_view basename, std::uint64_t size)
	{
		FileMetadata::validateBasename(basename);
		Encoder canonical;
		canonical.writeTextBytes("FTU-META-v1");
		canonical.writeString(basename);
		canonical.writeInteger<std::uint64_t>(size);
		const auto metadataCrc = checksum::Crc64::compute(canonical.encodedData());
		SessionId id;
		unsigned sourceBit = 0;
		for (unsigned bit = 0; bit < 128; ++bit)
		{
			if ((bit >= 48 && bit < 52) || bit == 64 || bit == 65)
			{
				continue;
			}
			const auto value = sourceBit < 64 ? (file_crc >> (63U - sourceBit)) & 1U
											  : (metadataCrc >> (121U - sourceBit)) & 1U;
			id.bytes[bit / 8U] |= static_cast<std::uint8_t>(value << (7U - bit % 8U));
			++sourceBit;
		}
		id.bytes[6] |= 0x80U;
		id.bytes[8] |= 0x80U;
		return id;
	}

	std::string SessionId::toString() const
	{
		static constexpr std::string_view DIGITS = "0123456789abcdef";
		std::string text;
		text.reserve(36);
		for (std::size_t i = 0; i < bytes.size(); ++i)
		{
			if (i == 4 || i == 6 || i == 8 || i == 10)
			{
				text += '-';
			}
			text += DIGITS[bytes[i] >> 4U];
			text += DIGITS[bytes[i] & 0xFU];
		}
		return text;
	}
} // namespace ftu::transfer::protocol
