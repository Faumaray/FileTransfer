#pragma once

#include "ftu/transfer/client/source_file.hpp"
#include "ftu/transfer/options.hpp"
#include "ftu/transfer/protocol/channel.hpp"
#include "ftu/transfer/protocol/file_metadata.hpp"
#include "ftu/transfer/protocol/message.hpp"
#include "ftu/transfer/protocol/progress.hpp"
#include "ftu/transfer/transport/connector.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace ftu::transfer
{
	class Client
	{
	public:
		Client(std::unique_ptr<transport::Connector> connector, Options options, std::filesystem::path path);
		void run();

	private:
		std::string attempt();
		protocol::Progress resume(protocol::Channel& channel, const protocol::Message& ready);
		std::uint64_t upload(protocol::Channel& channel, protocol::Progress start);

		std::unique_ptr<transport::Connector> m_connector;
		Options m_options;
		client::SourceFile m_source;
		protocol::FileMetadata m_metadata;
	};
} // namespace ftu::transfer
