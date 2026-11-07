#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** Asks for the character list again. The server answers creation with CharCreateOk alone and waits for this. */
inline void writeRequestGotoLobby(FWriteBuffer& buffer)
{
	buffer.writeUInt8(Opcode::extended);
	buffer.writeInt16(Opcode::requestGotoLobby);
}
}
}
