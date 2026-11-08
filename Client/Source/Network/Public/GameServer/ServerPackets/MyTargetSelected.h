#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** The server made something this client's target. Only the object is read, since a townsperson has no level colour to show. */
inline bool readMyTargetSelected(FReadBuffer& buffer, uint32& objectId)
{
	int32 value = 0;
	buffer.readInt32(value);
	if (buffer.hasFailed())
	{
		return false;
	}

	objectId = static_cast<uint32>(value);
	return true;
}
}
}
