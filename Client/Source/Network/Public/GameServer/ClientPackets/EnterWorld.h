#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** Eighty four bytes the server throws away, then twenty it reads as a traceroute. A shorter packet makes it read past the end and drop the entry. */
inline void writeEnterWorld(FWriteBuffer& buffer)
{
	buffer.writeUInt8(Opcode::enterWorld);
	buffer.writeZeros(84 + 20);
}
}
}
