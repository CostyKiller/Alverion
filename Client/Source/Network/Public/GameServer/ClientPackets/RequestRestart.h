#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** Leaves the world for the character list without dropping the link. The server answers RestartResponse, then CharSelectionInfo when it lets the character go. */
inline void writeRequestRestart(FWriteBuffer& buffer)
{
	buffer.writeUInt8(Opcode::requestRestart);
}
}
}
