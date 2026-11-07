#pragma once

#include "LoginServer/LoginServerProtocol.h"

namespace Network
{
namespace LoginServer
{
struct FServerList
{
	TArray<FServerListEntry> servers;

	bool read(FReadBuffer& buffer)
	{
		uint8 count = 0;
		uint8 lastServer = 0;
		buffer.readUInt8(count);
		buffer.readUInt8(lastServer);

		servers.Reset();
		for (int32 index = 0; index < count; index++)
		{
			FServerListEntry entry;
			uint8 address[4] = {};
			uint8 up = 0;

			buffer.readUInt8(entry.serverId);
			for (int32 part = 0; part < 4; part++)
			{
				buffer.readUInt8(address[part]);
			}

			buffer.readInt32(entry.port);

			// Age limit, PvP.
			buffer.skip(2);

			buffer.readUInt16(entry.currentPlayers);
			buffer.readUInt16(entry.maxPlayers);
			buffer.readUInt8(up);

			// Server type bits, brackets.
			buffer.skip(5);

			if (buffer.hasFailed())
			{
				return false;
			}

			entry.name = FString::Printf(TEXT("Server %u"), entry.serverId);
			entry.host = FString::Printf(TEXT("%u.%u.%u.%u"), address[0], address[1], address[2], address[3]);
			entry.status = (up != 0) ? EServerStatus::Online : EServerStatus::Offline;
			servers.Add(entry);
		}

		return true;
	}
};
}
}
