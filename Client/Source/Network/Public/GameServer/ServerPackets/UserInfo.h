#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** UserInfo's head, which is enough to know this client's own object id. */
inline bool readUserInfoObjectId(FReadBuffer& buffer, uint32& objectId)
{
	int32 id = 0;

	// Position and vehicle.
	buffer.skip(16);
	buffer.readInt32(id);

	objectId = static_cast<uint32>(id);
	return !buffer.hasFailed();
}
}
}
