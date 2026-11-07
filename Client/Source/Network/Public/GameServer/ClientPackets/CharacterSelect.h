#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
inline void writeCharacterSelect(FWriteBuffer& buffer, int32 slot)
{
	buffer.writeUInt8(Opcode::characterSelect);
	buffer.writeInt32(slot);
	buffer.writeZeros(2 + 12);
}
}
}
