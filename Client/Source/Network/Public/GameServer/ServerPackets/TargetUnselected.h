#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** The server cleared this client's target. The object it carries is the one that was selected. */
inline bool readTargetUnselected(FReadBuffer& buffer, uint32& objectId)
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
