#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
inline void writeValidatePosition(FWriteBuffer& buffer, const FVector& location, float yaw)
{
	buffer.writeUInt8(Opcode::validatePosition);
	buffer.writeInt32(toUnits(location.X));
	buffer.writeInt32(toUnits(location.Y));
	buffer.writeInt32(toHeightUnits(location.Z));
	buffer.writeInt32(yawToHeading(yaw));
	buffer.writeInt32(0);
}
}
}
