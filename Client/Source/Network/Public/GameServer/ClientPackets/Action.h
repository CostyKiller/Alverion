#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/**
 * A click on an object, which is how the server is asked to select it and, once it is selected, to deal with it.
 * The origin is where the player stands, and the action is zero for a plain click, which is the only kind this client sends.
 */
inline void writeAction(FWriteBuffer& buffer, uint32 objectId, const FVector& origin)
{
	buffer.writeUInt8(Opcode::action);
	buffer.writeInt32(static_cast<int32>(objectId));
	buffer.writeInt32(toUnits(origin.X));
	buffer.writeInt32(toUnits(origin.Y));
	buffer.writeInt32(toHeightUnits(origin.Z));
	buffer.writeUInt8(0);
}
}
}
