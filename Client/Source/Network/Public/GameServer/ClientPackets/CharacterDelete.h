#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
inline void writeCharacterDelete(FWriteBuffer& buffer, int32 slot)
{
	buffer.writeUInt8(Opcode::characterDelete);
	buffer.writeInt32(slot);
}
}
}
