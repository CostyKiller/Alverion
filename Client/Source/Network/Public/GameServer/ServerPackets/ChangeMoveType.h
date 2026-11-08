#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** ChangeMoveType: somebody started running or walking. The server numbers walking 0 and running 1. */
inline bool readChangeMoveType(FReadBuffer& buffer, uint32& objectId, bool& running)
{
	int32 object = 0;
	int32 type = 0;

	buffer.readInt32(object);
	buffer.readInt32(type);
	if (buffer.hasFailed())
	{
		return false;
	}

	objectId = static_cast<uint32>(object);
	running = (type != 0);
	return true;
}
}
}
