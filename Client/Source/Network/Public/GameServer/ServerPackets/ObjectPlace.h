#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** The object id and position every place packet starts with. MoveToLocation's is the destination. */
inline bool readObjectPlace(FReadBuffer& buffer, FObjectPlace& place)
{
	int32 objectId = 0;
	int32 x = 0;
	int32 y = 0;
	int32 z = 0;

	buffer.readInt32(objectId);
	buffer.readInt32(x);
	buffer.readInt32(y);
	buffer.readInt32(z);

	place.objectId = static_cast<uint32>(objectId);
	place.location = toLocation(x, y, z);
	return !buffer.hasFailed();
}

/** StopMove and ValidateLocation end in a heading. TeleportToLocation has a fade flag before it. */
inline bool readObjectPlaceWithHeading(FReadBuffer& buffer, FObjectPlace& place, bool teleport)
{
	int32 heading = 0;

	readObjectPlace(buffer, place);
	if (teleport)
	{
		buffer.skip(4);
	}

	buffer.readInt32(heading);
	place.yaw = headingToYaw(heading);
	return !buffer.hasFailed();
}
}
}
