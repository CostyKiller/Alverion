#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** Keyboard movement is mode zero, which is what makes the server take ValidatePosition as where the character is. */
inline void writeMoveToLocation(FWriteBuffer& buffer, const FVector& target, const FVector& origin)
{
	buffer.writeUInt8(Opcode::moveBackwardToLocation);
	buffer.writeInt32(toUnits(target.X));
	buffer.writeInt32(toUnits(target.Y));
	buffer.writeInt32(toHeightUnits(target.Z));
	buffer.writeInt32(toUnits(origin.X));
	buffer.writeInt32(toUnits(origin.Y));
	buffer.writeInt32(toHeightUnits(origin.Z));
	buffer.writeInt32(0);
}
}
}
